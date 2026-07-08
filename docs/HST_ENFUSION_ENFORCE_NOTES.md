# h-istasi Enfusion / Enforce Notes

Purpose: capture reusable facts learned while building h-istasi so we do not rediscover the same Enfusion and Enforce Script edge cases repeatedly.

This file is for practical engine/script behavior, not project planning. Keep entries concrete: what failed, why it failed, what works instead, and where the example lives.

## Player Identity And Roster UI

- Player display names are presentation-only.
  - Use `PlayerManager.GetPlayerName(playerId)` or `GetPlayerNameByIdentity(identityId)` to populate `HST_PlayerState.m_sDisplayName` when a player is seen.
  - Keep backend identity/SteamID64 values as the only authority for membership, commander, admin, loadout ownership, undercover state, and persistence references.
  - Member/commander UI should show `m_sDisplayName` as the row/action label and must not expose backend UUIDs, shortened UUIDs, `workbench_player_N`, or SteamID64 tokens in visible roster rows. Keep durable identity tokens internal to command arguments and debug/report evidence.
  - Current examples: `HST_PlayerLifecycleService.RefreshPlayerDisplayName()`, `HST_CommandUIService.BuildPlayerRosterName()`, and commander-transfer actions in the Members tab.

- Runtime admin grants from settings should be re-applied during every authoritative player refresh, not only on the first connection/spawn event.
  - `BackendApi.GetPlayerPlatformId(playerId)` can become available later than the initial player registration path. If a player is first registered under a fallback/backend identity and the SteamID64 appears later, a one-time admin check can leave them non-admin for menu/action requests.
  - Use the settings `adminIdentityIds` list for SteamID64 tokens only. Non-17-digit tokens are ignored and logged as diagnostics.
  - Current example: `HST_CampaignCoordinatorComponent.RefreshRuntimePlayerAuthority()` is used by setup, member, commander, admin, and visible-menu payload checks so the role state and the rendered menu snapshot agree.

- Player-owned UI components may be attached to either the player controller or the possessed entity depending on runtime context.
  - Local-owner checks should accept `GetGame().GetPlayerController()`, `SCR_PlayerController.GetLocalControlledEntity()`, `PlayerManager.GetPlayerIdFromControlledEntity(owner)`, and owner RPL-id lookup before deciding a component is not local.
  - If this is too strict, client input listeners such as the command menu `I` key never run because the component keeps returning before input polling.
  - Current examples: `HST_CommandMenuComponent.IsLocalOwner()` and `HST_CommandMenuRequestComponent.IsLocalOwner()`.

## Player Stamina And Screen Effects

- Player stamina writes must run on the owning client.
  - `CharacterStaminaComponent.GetStamina()` is readable from script, but `AddStamina()` must run on the owner. Server-only refill logic is unreliable on a dedicated server.
  - Use the player-owned request bridge to receive the authoritative `features.infiniteStaminaEnabled` setting and refill the local controlled entity's `CharacterStaminaComponent`.
  - The dark sprint overlay is `SCR_StaminaBlurEffect`, which drives `SuppressionVignette` and radial blur from the character `Exhaustion` signal. Suppress that effect with a modded class while infinite stamina is enabled instead of disabling unrelated damage/bleeding/drowning/poison/optic vignettes.
  - Current examples: `HST_CommandMenuRequestComponent.TickInfiniteStamina()` and `HST_StaminaBlurEffectPatch.c`.

## AI Groups And Game Master

- Use stock `SCR_AIGroup` population as the primary proof path for spawned squads.
  - Working AI-heavy mods follow the vanilla pattern: spawn an `SCR_AIGroup` prefab, configure max members when needed, call `SpawnUnits()` if the group does not spawn immediately, and use `GetAgentsCount()`/agent removal events for lifecycle decisions.
  - Direct character spawning plus manual attachment can be a recovery path, but it is not equivalent primary proof unless the attached characters become real `AIAgent` members of the displayed group.
  - Current example: `HST_PhysicalWarService.TrySpawnActiveGroup()` uses the stock group prefab and native delayed population before any degraded recovery path.

- A zero-agent group is only terminal after its delayed spawn queue is empty.
  - AI-force logic in external reference mods uses group agent count plus delayed queue count for active/spawning accounting, and deletes an empty group only when both `GetAgentsCount()` and the queue count are zero.
  - HST should make the same distinction: a stock group root with no current agents but `IsInitializing()` or a non-empty `GetSpawnQueueSize()` is still pending primary population, not an eliminated group.
  - Current examples: `HST_PhysicalWarService.IsActiveGroupNativeDelayedPopulationActive()` and `UpdateRuntimeGroupSurvivors()`.

- Game Master group Size comes from `SCR_EditableGroupComponent.GetSize()`, which delegates to `SCR_AIGroup.GetPlayerAndAgentCount()`.
  - If a visible group card reports Size 0 while soldiers are alive, prove both sides: the server group must have living `AIAgent` members, and each controlled entity's editable component should be parented under the group editable component.
  - `SCR_EditableGroupComponent.OnAgentAdded()` normally calls `SetParentEntity()` on each member; custom/manual population should reconcile that editable parent relationship and ensure the group has a living AI leader with `SetNewLeader()` if needed.
  - Current examples: `HST_PhysicalWarService.ReconcileRuntimeGroupEditableMembership()` and `BuildEditableGroupRuntimeEvidence()`.

- Active group state counts should be reconciled from live runtime agents after spawn.
  - A group root can exist with delayed native members while `m_iSpawnedAgentCount` and `m_iLastSeenAliveCount` are still zero, especially before the delayed-population callback or survivor update runs.
  - On routed/runtime updates, count living agents from the registered runtime group, finalize pending groups once agents are durable, update spawned/live/survivor counts, and rerun editable-membership reconciliation. This keeps command-menu summaries, markers, debug assertions, and Game Master group Size aligned with the visible soldiers.
  - Current examples: `HST_PhysicalWarService.ReconcileActiveGroupRuntimeMemberCounts()` and `HST_CommandUIService.BuildActiveGroupSpawnSummary()`.

- Response infantry speed should be enforced at the group and agent level.
  - `AIGroupMovementComponent.SetGroupCharactersWantedMovementType(EMovementType.RUN)` is the correct group-level control, but delayed-populated members can still appear after the first waypoint assignment.
  - Reapply response speed during routed updates, set tight formation displacement, and also set each live `AICharacterMovementComponent.SetMovementTypeWanted(EMovementType.RUN)` so support/QRF/Petros attack forces do not inherit a walking default.
  - Current examples: `HST_PhysicalWarService.ApplyResponseGroupMovementSpeed()` and `CampaignDebugIsActiveGroupResponseRunMovement()`.

- Non-deleting `SCR_AIGroup` roots need explicit terminal cleanup.
  - HST keeps active group roots from auto-deleting while delayed native member population is pending. Once every controlled member is dead, the group root should be unregistered/deleted so Game Master does not retain an active Size 0 group icon.
  - Delete the group root and HST runtime handles, but preserve dead character entities/corpses when they are not children of the group root.
  - Current example: `HST_PhysicalWarService.CleanupTerminalActiveGroupRuntimeCrew()`.

- Persistence smoke convoy rows are state-only restart sentinels.
  - They should not trigger physical convoy crew repair, stock-slot population, or direct fallback, because that contaminates primary-spawn certification with fake smoke groups.
  - Seed them with a non-fallback data-only mode and skip physical repair for smoke mission/group ids.
  - Current examples: `HST_PersistenceSmokeTestService.EnsureSmokeActiveGroup()` and `HST_PhysicalWarService.CanAttemptMissionConvoyCrewPopulationRepair()`.

## Runtime Architecture Patterns

- Civilian ambience should use the randomized CIV character prefab by default.
  - The runtime only needs generic civilian identity (`CIV_CHARACTER` / `CIV`)
    for town population, movement probes, heat, aid, and undercover systems.
    Explicit clothing-variant prefab lists create maintenance surface without
    adding current gameplay value.
  - Civilian vehicles should come from the CIV faction entity catalog at
    runtime. Keep the balance/config vehicle pool available as an override
    surface, but leave the default pool empty so catalog updates can flow into
    ambience without maintaining local prefab lists.
  - If the default catalog/config uses only `Character_CIV_Randomized.et`,
    keep `HST_CivilianService.MIN_CIVILIAN_CHARACTER_PREFABS` at `1`; otherwise
    civilian character spawning will self-block before runtime probes.
  - Current examples:
    `HST_DefaultCatalog.EnsureCivilianPools()` and
    `HST_CivilianService.SelectCivilianCharacterPrefab()`.

- HST-spawned vehicles should be source-catalog selected, but unclaimed.
  - Selecting a vehicle prefab from a faction, zone, garage, or runtime record is
    not the same as assigning engine faction ownership to the spawned entity.
    A vehicle with an affiliated faction can reject player entry/use when the
    player's faction is not friendly to that claim.
  - Clear `SCR_VehicleFactionAffiliationComponent`/`FactionAffiliationComponent`
    after spawning vehicles and during runtime repair. Keep faction stamping for
    group roots and crew characters only.
  - Runtime debug should check these as separate facts: crew/group faction must
    match the active group, while linked vehicles must have no claim.
  - Current examples:
    `HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive()`,
    `HST_VehicleRootPolicy.CountVehicleFactionClaimsRecursive()`, and
    `HST_PhysicalWarService.CountRuntimeVehicleClaimMismatch()`.
  - If a runtime faction repair pass starts from an AI group, convoy, carrier,
    or vehicle root, clear vehicle claims recursively before applying faction
    to crews. Use `HST_VehicleRootPolicy.IsVehicleRootLikeEntity()` to avoid
    stamping a vehicle root back to the active-group faction while still
    keeping infantry and group roots factioned.

- Unclaimed vehicles still need occupant rules when hostile AI are inside.
  - Clearing the vehicle faction claim keeps empty/captured vehicles usable by
    players, but it also means the engine will not automatically reject a player
    who tries to ride with hostile AI.
  - Enforce the rule at runtime from active-group state: if a non-resistance
    active group has living members inside its linked living vehicle, eject
    living player entities found in that same vehicle with
    `SCR_CompartmentAccessComponent.GetOutVehicle(EGetOutType.TELEPORT, -1,
    ECloseDoorAfterActions.INVALID, false, true)`. Do not set the vehicle's
    faction to solve this, because that reintroduces captured-vehicle lockout.
  - Current example:
    `HST_PhysicalWarService.EnforceOpposingOccupiedVehicleAccess()`.

- Convoy recovery should not undo deliberate AI dismounts.
  - Initial convoy spawn/staging may move unseated living crew near the vehicle
    and request driver binding, but once crew have been observed alive, contact
    or a fully dismounted living crew state should block recovery reseats.
  - Route reissue and route snap recovery must check the same suppression
    before moving crew back to the vehicle. Otherwise convoy AI can teleport
    back into seats after getting out to fight, or after a combat/contact phase
    has already made the convoy a battlefield object instead of a pure route
    object.
  - Current examples:
    `HST_PhysicalWarService.ShouldSuppressMissionConvoyCrewReseat()`,
    `EnsureMissionConvoyCrewSeating()`, and
    `TrySnapMissionConvoyVehicleToRoute()`.

- Active mission target zones are physical objectives, not ordinary render-bubble zones.
  - If an active mission has a target zone and a real runtime primitive, the
    physical-war service should keep that zone active even when no player is
    near it. Exclude state-only `abstract_fallback` missions and convoy
    primitives, because those have separate runtime controllers.
  - Mission-owned active groups identified by `m_sMissionInstanceId` are mission
    projections, not abstract garrison population. Exclude them from garrison
    accounting, survivor fold-back, and normal zone deactivation cleanup.
    Non-convoy mission groups should be deleted after the owning mission is no
    longer active; convoy groups stay under the convoy runtime path.
  - Current examples:
    `HST_PhysicalWarService.ShouldForceMissionTargetZonePhysical()`,
    `HST_PhysicalWarService.IsMissionOwnedActiveGroup()`,
    `HST_PhysicalWarService.CleanupInactiveMissionOwnedActiveGroups()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugRenderBubbleMissionTargetCase()`.

- Planning/checklist docs should be first-party h-istasi documents.
  - When converting external planning material, keep feature status, gaps,
    priorities, implementation contracts, and acceptance tests.
  - Strip external project/source names, comparison tables, external file
    paths, citation blocks, and source-mapping language before committing docs.
  - Keep third-party attribution separate and do not use planning docs as a
    place for source provenance.

- Do not assign `new` into a non-`ref` managed object parameter.
  - Enforce treats a method parameter such as `HST_FooResult result` as a weak
    reference for assignment purposes. `result = new HST_FooResult()` fails with
    "Variable is not strong ref". If a factory/failure helper accepts an
    optional existing result, create a fresh local result in the null branch and
    return it instead of assigning into the parameter.
  - Current examples:
    `HST_ForceCompositionService.Fail()` and
    `HST_SpawnPlacementService.Fail()`. The same pattern applies to
    `HST_CampaignCoordinatorComponent.FailMissionCategorySelection()`.

- Enforce class scope does not allow duplicate helper method declarations.
  - Even identical helper bodies in different parts of a large component fail
    Game script compilation as a multiple declaration. Reuse one class-local
    helper for shared debug lookups instead of adding a second same-named
    helper near a new proof case.
  - Current example:
    `HST_CampaignCoordinatorComponent.FindCampaignDebugEnemyOrderInState()`.

- Enforce method signatures cap out at 16 arguments.
  - Large catalog/import helpers should derive secondary metadata internally or
    take a compact object instead of carrying every field as a positional
    parameter. Exceeding the cap can also cascade into misleading follow-up
    parser errors such as duplicate parameter declarations on later methods.
  - Current example:
    `HST_DefaultCatalog.UpsertEveronLocationPlanZone()` derives runtime
    composition/spawn metadata inside the helper, with mission-site taxonomy
    anchors forced to `spawn_none`.

- Enemy abstract orders should be resolved as named, auditable outcomes.
  - Strategic enemy actions that do not need live AI still need the same
    durable order lifecycle as physical orders: spend resources, store target
    source/position, resolve through commander code, stamp resolution kind, and
    persist the resulting garrison/town/support pressure.
  - Debug helpers should resolve a specific order id when proving a fixture.
    Resolving every due active order can mutate unrelated live campaign state
    and makes one-click evidence harder to trust.
  - Current examples:
    `HST_EnemyCommanderService.DebugResolveOrderNow()`,
    `HST_EnemyCommanderService.ApplyResolvedOrder()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugEnemyOrderResolutionCase()`.

- Enemy target choice should be scored through a reusable result object.
  - Inline score-and-roll code is hard to audit and easy to desynchronize from
    debug proofs. Build an `HST_EnemyTargetScoreResult` with eligible
    candidates, selected/best ids, weights, and component reasons, then let
    production selection and one-button debug assertions consume the same path.
  - Hideouts and mission-site anchors are bookkeeping locations, not strategic
    attack targets. Exclude them before scoring even if they have high priority
    metadata for marker/site generation.
  - Current examples: `HST_EnemyCommanderService.BuildTargetScoreResult()`,
    `HST_EnemyCommanderService.BuildEnemyTargetScoreReport()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugEnemyTargetScoringCase()`.

- Active force rows need durable source ownership and baseline force counts.
  - Live manpower is mutable: groups can be queued, spawned, damaged, folded,
    deleted, or represented abstractly while the owning gameplay source still
    needs to resolve cleanup, refunds, garrison return, and mission/support
    status.
  - Store the source id on the active group when it is created: mission
    instance, support request, garrison zone, or QRF instance. Also store the
    original infantry and vehicle counts separately from survivor/current
    counts so fold-back and debug evidence can distinguish planned force size
    from remaining force size.
  - Migration should backfill original counts from current counts and recover
    source links from existing support requests, QRF rows, mission group ids,
    or zone id as a garrison fallback.
  - Current examples:
    `HST_SupportRequestService.ApplyActiveSupport()`,
    `HST_PhysicalWarService.CreateActiveGroup()`,
    `HST_CampaignSaveData.NormalizeActiveGroupSourceLinks()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugPhysicalResponseFoldbackCase()`.

- Mission completion rewards and failure penalties should be proved through the
  coordinator wrappers, not by directly setting mission status.
  - `CompleteMission()` progresses objectives, transitions mission status, then
    routes configured rewards and category-specific outcomes through
    `HST_StrategicService.ApplyMissionOutcomeEvent()`. This service records a
    durable `HST_StrategicEventState` row and applies mission money/HR, town
    support, capture progress, aggression, enemy-resource, and HQ-knowledge
    consequences from one path.
  - `FailMission()` similarly transitions mission status without applying the
    mission-service aggression write directly, then routes configured failure
    aggression, town-support loss, HQ-knowledge changes, and linked objective
    failure through the coordinator wrapper plus strategic-event service.
  - `HST_MissionService.Tick()` should only transition active missions into
    expired state and expose ids that expired during that tick. The coordinator
    sweep then calls
    `HST_StrategicService.ApplyMissionExpiryEvent()` so expiry aggression is
    recorded as a durable `mission_expired` row. Skip the generic expiry
    penalty for the HQ defense mission because its timer expiry is a success
    condition handled by the defense outcome path.
  - Debug proofs that seed a mission fixture should snapshot and restore the
    mission row, strategic-event rows, economy totals, target-zone
    owner/support, relevant enemy resource/aggression pools, and marker state
    after the save-data roundtrip check. Prefixed debug cleanup must remove
    strategic events linked by event id, source id, mission instance id, target
    zone id, reason, or summary so one-button runs do not accumulate stale
    proof rows.
  - Current example:
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugMissionCompletionRewardCase()`
    and `HST_CampaignCoordinatorComponent.BuildCampaignDebugMissionFailurePenaltyCase()`.

- Durable strategic-event rows should store deltas, not replay-only prose.
  - `HST_StrategicEventState` captures the event kind, mission/source identity,
    target zone/faction, applied status, before/after owner fields, and numeric
    deltas for money, HR, town support, capture progress, aggression,
    attack/support resources, and HQ knowledge.
  - Use the service report and Full Campaign Debug assertions to prove both the
    durable event row and the actual campaign mutation. A report row without a
    matching state delta is evidence only; it should not be counted as a
    gameplay consequence.
  - Current examples:
    `HST_StrategicService.ApplyMissionOutcomeEvent()`,
    `HST_StrategicService.BeginZoneCaptureEvent()`,
    `HST_StrategicService.BuildStrategicEventReport()`,
    and the mission completion/failure strategic-event assertions in
    `HST_CampaignCoordinatorComponent`.
  - Zone capture should bracket the real capture path with a strategic event:
    begin before owner change, complete after garrison seed, aggression,
    counterattack evaluation, and linked town support. Full Campaign Debug
    Phase 17 should assert the `zone_captured` row and save-data roundtrip, not
    just the final owner field.
  - Convoy outcome branches should bracket their existing reward/penalty code
    with `HST_StrategicService.BeginConvoyOutcomeEvent()` and
    `CompleteStrategicEvent()`. Use the mission instance as the event's mission
    identity, the convoy asset id as the source id when an asset caused the
    outcome, and the actual support town as the target zone when town support
    changes. Full Campaign Debug should keep an isolated convoy delivery proof
    that asserts the asset flag, HR/support deltas, strategic-event row, save
    roundtrip, and cleanup.
  - Hostile support resolved near HQ should use
    `HST_StrategicService.BeginSupportNearHQEvent()` before applying the HQ
    knowledge gain, then complete the event after `HST_HQService.AddHQKnowledge()`.
    Keep the radius modest at 700m and the default knowledge gain at 4 so support
    activity near HQ is explainable without making every nearby support action a
    fast Defend Petros trigger. Full Campaign Debug should keep an isolated
    `support_near_hq.strategic_event.contract.runtime` proof that asserts support
    resolution, garrison outcome, HQ-knowledge delta, strategic-event row, save
    roundtrip, and cleanup.

- Undercover vehicle cover should be answered from campaign state, not only
  from the currently controlled entity.
  - Track reported/hot vehicle state on `HST_RuntimeVehicleState`: reported
    flag, heat value, report expiry, last report reason/zone, civilian-cover
    eligibility, and passenger compromise count.
  - Eligibility can still inspect the live vehicle entity when present, but a
    tracked hot/reported runtime vehicle must block civilian vehicle cover even
    if the current physical entity lookup is unavailable later.
  - Current examples: `HST_CivilianService.RegisterVehicleHeat()`,
    `ResolveRuntimeVehicleUndercoverReason()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugVehicleHeatCase()`.

- Physical spawn placement should go through an explainable request/result contract.
  - Tactical systems should ask for source/target placement by intent, target
    zone, preferred source, standoff, road preference, dry-ground requirement,
    vehicle-safe requirement, and HQ standoff rule.
  - Keep the placement result data-only: resolved spawn/target positions,
    road forward vector, road/dry/vehicle-safe booleans, distance metrics, and
    explicit failure reason. Runtime services can then decide whether to spawn,
    fallback, or resolve abstractly.
  - Current examples: `HST_SpawnPlacementService`,
    `HST_SupportRequestService.ApplyActiveSupport()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugSpawnPlacementCase()`.

- Enemy support spending should go through a ledger-backed defense gate.
  - A plain resource check is not enough for QRF/support pacing because it cannot
    explain why a zone did or did not receive help, cannot prevent same-zone
    stacking, and cannot safely refund survivor fold-back.
  - Keep proactive attack pressure and reactive support spending separate.
    Proactive background-war attacks, support pressure, and Petros pressure
    should spend attack resources only. Capture-triggered counterattacks, QRFs,
    rebuilds, roadblocks, and direct enemy support requests should spend support
    resources through the support ledger.
  - Keep the ledger data-only: faction, zone, recent damage score, attack/support
    spent, cooldown, refund totals, and last decision reason. Runtime services
    should call the gate before spending and let reports read the ledger instead
    of reconstructing decisions from orders or support requests.
  - Current examples: `HST_EnemyDirectorService.CanSpendDefense()`,
    `HST_EnemyCommanderService.QueueOrder()`,
    `HST_PhysicalWarService.UpdateQRF()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugEnemySupportSpendCase()`.

- Physical support groups need an owner-controlled fold-back path.
  - Ground support/QRF groups are not normal zone garrisons, so generic zone
    deactivation intentionally skips them. The support lifecycle must fold
    survivors when the target is no longer inside an active player/objective
    event bubble, delete the live runtime group handle, and keep the persistent
    group row terminal long enough for support and enemy-order synchronization.
  - Fold-back should return only the active group's recorded survivor infantry
    and vehicle counts to the abstract garrison, then preserve the folded group
    row and returned garrison strength through save-data copy before any debug
    cleanup restores the fixture state.
  - Current examples: `HST_SupportRequestService.FoldPhysicalSupportOutsideBubble()`,
    `HST_PhysicalWarService.FoldActiveSupportGroup()`,
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugPhysicalResponseFoldbackCase()`,
    and `HST_CampaignCoordinatorComponent.BuildCampaignDebugGarrisonFoldbackCase()`.

- Physical support deployment proof belongs on durable request/group state.
  - `HST_SpawnPlacementResult` is the source of truth for route-aware support
    staging evidence: placement type, selected spawn/target positions,
    target/road/HQ distances, road resolution, dry-ground and vehicle-safe
    checks, player/active-AI clearance checks, and debug summary.
  - Copy the deployment route id and placement proof onto
    `HST_SupportRequestState`, then copy the same route and force counts onto
    the linked `HST_ActiveGroupState`. `HST_CampaignSaveData` should roundtrip
    those fields so folded support can be audited after save/load without live
    `IEntity` references.
  - If a vehicle-capable support composition cannot find vehicle-safe staging,
    downgrade to infantry-only deployment instead of losing the whole response.
  - Player-selected support destinations should remain the support request and
    active-group target. Resolve the physical spawn as an offset staging point
    around that destination, reject candidates near living players or nonterminal
    active groups, and route the spawned group toward the selected destination
    instead of replacing the destination with the spawn point.
  - Current examples: `HST_SupportRequestService.ApplyActiveSupport()` and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugPhysicalResponseFoldbackCase()`.

- Routed response groups should consume generated route waypoint chains before
  adding more runtime AI controls.
  - For support/QRF active groups, resolve the route id against generated
    routes, fall back through the target zone's route ids, then use the
    generated `route_<zone>_alpha` convention before falling back to direct
    source-target movement.
  - Keep generated route positions available while the runtime AI group is still
    pending native population, but persist assigned AI waypoint count only after
    the group is populated. Move live entities only after the group is spawned.
  - Current example: `HST_PhysicalWarService.UpdateActiveGroupRoutes()`.

- Routed infantry AI waypoints need explicit lifecycle ownership.
  - Assign route waypoints only after the active group's runtime `AIGroup` has
    populated. Use generated-route positions as the waypoint chain, skip the
    group's current/source position, and require at least two assigned waypoint
    entities before marking the active group with `infantry_waypoints`.
  - Use patrol waypoints for intermediate route legs and a search/sweep waypoint
    for the final target position; mark the group with `infantry_sweep` only
    after the final target waypoint is actually assigned.
  - Track waypoint entities by active group id and delete them in the same
    cleanup path as runtime crew entities. Otherwise repeated debug runs or
    fold-back cleanup can leave orphan waypoint entities in the world.
  - Once infantry AI waypoints are assigned, keep abstract route progress in the
    active-group state for fold-back/accounting, but do not keep teleporting the
    runtime group root along that abstract route.
  - Current examples: `HST_PhysicalWarService.AssignActiveGroupInfantryRouteWaypoints()`
    and `HST_PhysicalWarService.DeleteRuntimeGroupWaypoints()`.
  - One-button route certification cannot assume the native delayed-spawn
    callback fires during the same admin command. Keep primary population proof
    strict in `CampaignDebugResolvePendingActiveGroupPopulation()`, then let a
    separate route resolver force the production direct-infantry fallback and
    call the normal route updater before asserting move/sweep waypoints.
  - Current example: `HST_PhysicalWarService.CampaignDebugResolveActiveGroupRouteAssignment()`.

- Mixed response vehicles should use the force-composed vehicle prefab, not a
  late random reselection.
  - Persist the selected vehicle prefab on `HST_ActiveGroupState` when applying
    force composition, then spawn the linked runtime vehicle only after the
    infantry `AIGroup` has populated or finalized.
  - Track the vehicle under the same active group id so existing runtime group
    cleanup deletes crew, route waypoints, and the attached vehicle together.
  - Current example: `HST_PhysicalWarService.TrySpawnActiveGroupAttachedVehicle()`.

- Enemy faction checks should use campaign relation helpers, not broad
  "not resistance" tests.
  - `HST_FactionRelationService.ResolveRelation()` distinguishes same faction,
    resistance-vs-enemy, rival enemy, neutral, and unknown relationships.
  - Enemy commander target scoring should record owner relation and explain the
    owner-score component (`resistance_control`, `owned_zone_pressure`,
    `rival_enemy_pressure`, or `neutral_pressure`).
  - Defense pressure ledgers, rebuild, roadblock, and QRF behavior should only
    run for same-faction holdings. Rival-held zones may be attacked, but they
    are not a garrison-rebuild target for the evaluating faction.
  - Full Campaign Debug should prove order-type decisions separately from
    target scoring. Controlled relation fixtures should cover resistance-held
    counterattack, same-faction QRF/rebuild/roadblock, and rival-held support
    call behavior.

- Town political support should be event-ledger backed.
  - Directly changing FIA/occupier support, reputation, heat, population, or
    police/roadblock pressure hides why a town flipped and makes save/reload
    regressions hard to diagnose.
  - Store a data-only influence event with explicit deltas, created/expiry
    seconds, source, and reason, then derive town support, undercover
    restriction, population killed/remaining, and active/expired modifier
    counts from that ledger.
  - Current examples: `HST_CivilianService.RegisterInfluenceEvent()`,
    `HST_CivilianService.RegisterIncident()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugTownInfluenceLedgerCase()`.

- Force creation should start from a serializable request/result contract.
  - Tactical systems should ask for a force by intent, faction, war level,
    budget, manpower bounds, vehicle permissions, and reason, then consume a
    result containing selected group/vehicle plans, skipped prefab counts,
    cost, manpower, selected tier, and a compact debug summary.
  - Keep this contract data-only. Store composition ids, intent, summary,
    counts, and failure reasons on support/order/active-group records, not
    `IEntity`, `AIGroup`, vehicle, or waypoint handles.
  - A runtime debug suite should prove low/high war-level scaling,
    vehicle-disabled requests, invalid prefab failures, unsupported capability
    failures, and support/order/active-group serialization before physical
    spawn behavior is judged.
  - Current examples: `HST_ForceCompositionService`,
    `HST_SupportRequestService.ApplyActiveSupport()`,
    `HST_MissionRuntimeService.ComposeMissionGuardForce()`,
    `HST_GarrisonService.ComposeGarrisonForce()`,
    `HST_EnemyCommanderService.SyncPhysicalizedOrder()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugForceCompositionCase()`.

- Treat fallback use as a primary-method failure that must be visible in debug output.
  - A fallback can keep a test moving, but the runtime certification path should still report which primary method failed, why it failed, and which fallback took over.
  - Avoid report wording that makes fallback success look equivalent to primary success. The useful proof is: primary requested, primary observed, fallback not used.

- Long-running AI work should go through a spawn queue or force-owned request layer.
  - A practical AI-heavy pattern is: create data-only spawn requests, enqueue them, preprocess just before execution, cancel invalid/excessive requests, and process within a small per-poll time budget.
  - Attach spawned groups to an owning force so group attach/detach, agent add/remove, kill, queued spawn count, streamed-in/out state, and cleanup can be reported from one authority instead of being re-derived by every gameplay service.
  - HST currently does this work mostly inside `HST_PhysicalWarService`; future refactors should move toward a reusable AI force/spawn queue service while keeping current debug proof labels.

- Streamed-out AI should keep explicit simulated manpower instead of pretending the live group still exists.
  - Store surviving member prefab identities and proxied manpower before deleting a live group root for distance/streaming reasons.
  - On stream-in, rebuild through the same stock group spawn path and reattach the rebuilt group to its owning force.
  - This keeps performance, persistence, and Game Master state aligned: live groups are live, streamed-out groups are simulation records, and terminal dead groups are cleanup candidates.

- Zone ownership, battle areas, construction areas, and illegal/player-danger areas are cleaner as components than as repeated radius checks.
  - A zone component can own shape/polygon containment, map color/style, faction affiliation, parking/traffic effects, and tags such as battle, POI, construction, or blocked traffic.
  - A controller component can link nearby shape zones to a POI, refresh them when faction/battle state changes, and replicate only the visible zone state.
  - HST zones are currently data records plus marker/proximity services; if construction, traffic exclusion, front lines, or richer battle areas expand, introduce a zone component/controller layer instead of adding more ad hoc radius checks.

- Player command readiness depends on vanilla player faction and group state, not just HST membership/admin state.
  - After player handover/spawn, ensure the player controller's faction affiliation is set through the faction manager path and that the vanilla player group/controller group id are coherent.
  - If the groups manager has a player group but `SCR_PlayerControllerGroupComponent` does not, repair the controller's group id and replicate it.
  - This is separate from HST command-menu permissions: a player can be an HST admin and still have broken vanilla command context if faction/group handover did not finish.

- Campaign mechanics become easier to test when runtime entities own their local state.
  - Town support/threat, wanted heat, build-area contents, logistics storage, respawn tickets, garages, and missions can be component-owned and aggregated by manager systems.
  - The manager should answer campaign-wide questions and emit reports; the component should own lifecycle hooks, replication, persistence, and local invariants.
  - HST currently centralizes many of these as campaign-state rows; that is debuggable, but future feature growth should prefer component-local state for mechanics that are physically represented in the world.

## UI Layouts

- Top-level UI layouts should be created with the workspace as the parent when they are meant to exist above game/map UI:
  - Use `workspace.CreateWidgets(LAYOUT, workspace)`.
  - This matches native UI patterns better than creating a top-level root with no parent.
  - Creation-time geometry can still report `0x0`; schedule a post-layout check before trusting bounds.

- Creation-time widget geometry is often not final.
  - `GetScreenSize` and `GetScreenPos` can return `0x0` immediately after `CreateWidgets`.
  - Use `GetGame().GetCallqueue().CallLater(..., 0, false)` and often a second pass around `50 ms` later for diagnostics or final visibility/z-order reassertion.
  - Current examples: `setup_prompt_ready`, `setup_confirm_modal_ready`, `notification_toast_ready`, `command_action_dialog_ready`, `mission_report_dialog_ready`, `command_menu_ready`, `loadout_editor_ready`.

- Ready diagnostics need to check more than layout creation.
  - A layout can be created and still be unusable because expected children are missing, hidden in hierarchy, or still reporting `0x0` bounds after anchor resolution.
  - Use `HST_UIDebug.LogReadyWidgetsCsv` in delayed ready passes beside geometry logs so playtest logs identify the exact failed widget state.
  - Negative sizes are different from delayed `0x0` layout resolution. Ready logs now separate `zero=`, `negative=`, and `offscreen=` so bad anchor signs are obvious.

- Row/list containers need post-layout child samples when debugging generated UI.
  - Creation-time row logs confirm data population, but list rows can still resolve into the wrong parent, wrong size, or wrong z-order after layout.
  - Use `HST_UIDebug.LogNamedChildSummaryCsv` during delayed ready passes for dynamic list hosts such as command tabs/main/actions and loadout slot/candidate/storage lists.
  - This logs the parent bounds plus the first few child widget bounds after Enfusion has settled anchors.

- Generated row and tile dimensions should live in the row layout resource.
  - Script-side `SizeLayoutWidget` overrides on every generated row reintroduce viewport-dependent geometry logic and can fight wrap/scroll hosts.
  - Prefer fixed `SizeLayoutWidget` dimensions in the row layout and let the parent `VerticalLayoutWidget` or `WrapLayoutWidget` place children.
  - Script should create the row, bind user ids, set colors/text/images, and leave row size to the layout.
  - Current example: `HST_LoadoutCandidateTile.layout` owns storage candidate tile width, height, and text bounds.

- Layout diagnostics must be runtime-gated.
  - Compile-time `static const` feature flags are useful as a hard kill switch, but every high-volume helper should also check the debug setting loaded from runtime settings.
  - Cache the loaded runtime setting in the debug helper so row/widget probes do not parse settings on every row.

- EDDS metafiles must have matching EDDS resources.
  - Runtime symptom: `RESOURCES (E): metafile without corresponding resource`.
  - If an icon is regenerated under a new path, delete any stale `.edds.meta` left behind at the old path; Workbench still scans it and reports the missing resource even when scripts reference the new GUID.
  - Current example: loadout editor search uses `Assets/512/search_icon.edds`; the old `Assets/1254/Search Icon.edds.meta` had no matching `.edds`.

- `root.FindAnyWidget(root.GetName())` may not find the root itself.
  - If debug code checks expected widgets and includes the root name, explicitly compare `root.GetName()` before reporting the root missing.

- A `ButtonWidget` does not accept arbitrary child widgets.
  - Runtime symptom: `GUI (E): Cannot add a child, the ButtonWidget CloseButton does not accept more children`.
  - Avoid putting custom `FrameWidget`/`PanelWidget`/`TextWidget` children inside a `ButtonWidget` unless the engine-provided slot pattern is known to work for that exact widget.
  - Stable workaround: make the button a real sibling hit target and place a sibling `TextWidget` over it with `Ignore Cursor = true`.
  - If a label renders blank while the button hit target still appears, first check whether the label is nested under the button. Move filter/sort/action labels out as siblings before chasing script-side text population.
  - Current examples: `HST_CommandMenu.layout` close button, `HST_SetupConfirmModal.layout` Yes/No labels, `HST_ActionDialog.layout` Cancel/Confirm labels, `HST_ReportDialog.layout` Close label, `HST_LoadoutEditor.layout` remove/filter/sort labels.

- If a layout-owned button does use a supported `ButtonWidgetSlot` child body, that body must fill the button slot.
  - Runtime symptom: the button rectangle is visible and clickable, but child labels report `0x0` or negative height and render blank.
  - Put `HorizontalAlign 3` and `VerticalAlign 3` inside the `Slot ButtonWidgetSlot {}` block for the body frame.
  - Current example: `HST_LoadoutEditor.layout` template/settings button bodies.

- Scripted layout bindings should only name widgets that actually exist in the layout resource.
  - Helpers such as `ConfigureStorageBrowserButton` can fail quietly for optional visual parts because `FindAnyWidget` returns null without a script error.
  - For layout-owned controls, keep hit targets, labels, accents, and other visual siblings named in the layout and validate those names against script calls.
  - If the button itself owns the background color, pass the button name for the background binding instead of inventing an unmatched `*Background` name.
  - Current example: `HST_LoadoutEditorComponent.ConfigureStorageBrowserButton` binds storage filter/sort/search buttons to sibling label/accent widgets in `HST_LoadoutEditor.layout`.

- Icon-only controls need text fallbacks wired from script.
  - A row layout can include a hidden fallback text widget, but it stays blank unless script populates and shows it when image loading fails.
  - Runtime symptom: storage/search tabs or controls appear as visible clickable buttons with no readable label if the icon resource is missing or not ready.
  - Current example: `HST_LoadoutEditorComponent.RenderStorageCategoryTabs()` sets `Fallback` through `GetStorageBrowserCategoryFallback()` when `SetLoadoutImageTexture()` fails.

- Fixed-width generated tab rows must be recalculated when the tab count changes.
  - Runtime symptom: a newly added storage/search tab is populated in logs but does not appear because the row prefab still has the wrong tab count width and padding.
  - For six storage browser tabs in the current right pane, `HST_LoadoutStorageCategoryTab.layout` uses compact tab cells and the host row uses symmetric insets so the tab strip stays centered after removing a category.

- Slot alignment keys must live inside the `Slot ... {}` block for the widget slot that owns them.
  - Runtime symptom: `GUI (E): Unknown keyword/data 'HorizontalAlign'` or `VerticalAlign` while loading the layout resource.
  - Do not put `HorizontalAlign` / `VerticalAlign` directly on a layout widget body such as `HorizontalLayoutWidgetClass`; move them into the slot block or remove them if the slot already stretches.
  - Current example: `HST_LoadoutEditor.layout` `TopTabItems`.

- Full-screen anchored roots should not be manually resized.
  - Avoid `FrameSlot.SetPos(root, 0, 0)` and `FrameSlot.SetSize(root, ...)` on roots using stretched anchors.
  - Runtime warning pattern: position/size only works when min/max anchors are the same.
  - Let layout anchors own full-screen geometry; script may set z-order, visibility, opacity, and text/data.

- Fixed-size centered modal panels should use the same `SizeX` / `SizeY` plus alignment pattern as working native dialogs.
  - Use `Anchor 0.5 0.5 0.5 0.5`, `PositionX 0`, `OffsetLeft 0`, `PositionY 0`, `OffsetTop 0`, `SizeX width`, `OffsetRight -width`, `SizeY height`, `OffsetBottom -height`, and `Alignment 0.5 0.5`.
  - A setup confirmation modal using direct center-relative offsets such as negative left/top and positive right/bottom created the layout and populated text, but the dialog resolved to `0x0` and buttons had unusable or negative bounds.
  - Current examples: `HST_SetupConfirmModal.layout`, `HST_ActionDialog.layout`, `HST_ReportDialog.layout`.

- `OffsetRight` and `OffsetBottom` signs depend on whether a slot has explicit `SizeX` / `SizeY`, whether that axis stretches, and which side the fixed anchor uses.
  - Slots with explicit `PositionX` / `PositionY` plus `SizeX` / `SizeY` often serialize the far edge as negative, such as a centered modal using `SizeX 620` with `OffsetRight -620`.
  - Top/left same-anchor fixed boxes without explicit size need negative far edges, such as `Anchor 0 0 0 0`, `OffsetLeft 116`, and `OffsetRight -560`. Positive far edges produced negative runtime sizes for top tabs, left buttons, left rail, and footer chrome.
  - Top/left same-anchor fixed-height children also need explicit `SizeY` with a negative far edge, such as `OffsetTop 72`, `SizeY 78`, and `OffsetBottom -150`. Positive far edges can collapse tab strips and text bands even when the parent panel is visible.
  - Stretched inset panels need positive far-edge margins, such as `Anchor 0 0 1 1`, `OffsetLeft 240`, and `OffsetRight 524`. Negative far-edge margins made command-menu center and right panels grow underneath sibling panels.
  - Right or bottom anchored fixed boxes can use negative left/top offsets to define size while keeping positive right/bottom offsets inside the parent.
  - Runtime symptom when this is wrong: delayed ready logs show negative widget sizes or panels wider/taller than their parent, such as left rails, navigation panels, top tabs, command-menu center panels, or footer hints resolving to impossible bounds.
  - Current examples: `HST_SetupConfirmModal.layout`, `HST_CommandMenu.layout`, `HST_LoadoutEditor.layout`, loadout row layouts.

- Mode-owned header/action/settings controls need fixed-height slots just like visible chrome.
  - Workbench can show top-anchored header text, action buttons, or settings rows that use positive `OffsetBottom` values, but runtime can hide or collapse them when the mode panel is toggled and repopulated.
  - Use explicit `SizeY` and a negative far edge for candidate headers, remove buttons, and settings rows; keep stretched anchors only for containers that truly fill their parent.
  - Current example: `HST_LoadoutEditor.layout` candidate header/remove controls and settings rows.

- Layout fragments intended to fill a parent slot should use stretched anchors at their root.
  - A root `FrameWidget` with `Anchor 0 0 0 0` can resolve to `0x0` when created inside a dynamically populated placeholder.
  - Use `Anchor 0 0 1 1` for reusable item/preview fragments and let the parent list, tile, or placeholder own the actual dimensions.
  - Current example: `HST_LoadoutItemPreviewCell.layout`.

- Keep visual children passive unless they are real controls.
  - Use `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` or layout `Ignore Cursor = true` for passive panels, labels, overlays, and notification visuals.
  - Do not place invisible full-screen widgets unless they are intentional visible modal roots with real content.

- Widget event callbacks can pass a null `Widget`.
  - `ScriptedWidgetEventHandler` methods must guard `!w` before calling `w.GetUserID()` or passing the widget into component logic.
  - Runtime symptom when this is wrong: VM exception in `OnMouseButtonUp` with a null variable named `w`.

- Full-screen render targets need a repeatable sibling layer order.
  - Keep the render target container low in the root z-order and reassert visible UI panels above it after the layout has resolved.
  - Schedule a delayed ready pass for final geometry logs because render-target and UI-layer widgets can report `0x0` during creation.
  - Current example: `HST_LoadoutEditor.layout` with `ApplyLoadoutLayerOrder` and `loadout_editor_ready` diagnostics.

- Complex full-screen shells also benefit from one repeatable layer-order pass.
  - Keep geometry in the layout file, but reassert sibling z-order for dimmers, shell surfaces, panels, headers, and overlaid button labels after creation and delayed layout resolution.
  - Current example: `HST_CommandMenu.layout` with `ApplyCommandMenuLayerOrder` and `command_menu_ready` diagnostics.

- Long-lived screen roots should be reused while open.
  - Recreating a workspace-parented layout root during data refresh can leave input state, scroll state, and delayed layout diagnostics racing against a removed shell.
  - Keep the top-level root cached for screens such as the command menu, then clear only dynamic row/list hosts with `while (container.GetChildren()) container.GetChildren().RemoveFromHierarchy()`.
  - Current example: `HST_CommandMenuComponent.EnsureMenuRoot`, `ClearDynamicMenuRows`, and `ClearMenuContainerChildren`.

- Command-menu debug/report visibility should be server-authoritative.
  - Gate the Admin tab and report/debug action payloads from `HST_CommandUIService.BuildVisibleMenuPayload()` using `HST_RuntimeSettings.m_Debug.m_bDebugMenuEnabled`; do not rely on the client fallback tab list or admin role alone.
  - `ExecuteVisibleCommand()` should also reject debug/report command ids when the debug menu is disabled so manually submitted visible-menu ids cannot bypass hidden UI.
  - If a normal gameplay action previously used a `*_report` command id, expose a non-report gameplay id for the regular menu path so turning off the debug menu does not remove real gameplay controls.
  - Current examples: Overview undercover request/clear stays visible, Admin is omitted when debug menu is disabled, and Forces training uses `train_troops` instead of `train_troops_report`.
  - Petros/HQ should not expose the old hardcoded HQ relocation buttons in the normal command menu. Setup-map HQ placement owns initial placement, and `phase23.ui.no_hq_move_menu_actions` plus foundation validation should catch accidental reintroduction of visible `Move HQ` rows.
  - HQ relocation after setup belongs to Petros' world-space context action, not the visible menu payload. Keep `Relocate HQ` as Petros' final selectable action, flip the owner-local label to `Deploy HQ Here` through the player-owned request bridge while relocation is active, and assert the final selectable runtime action through `ActionsManagerComponent.GetActionsList()`.
  - Campaign-debug rendered UI proof should be owner-client reported instead of inferred from server payloads. Dispatch through the player-owned request bridge, open the command menu on the owner client, wait for post-snapshot layout, count expected visible non-zero widgets, and send the report back to the coordinator as a `CLIENT_RENDERED` assertion.

- HQ arsenal user-action order must be proved from the runtime action manager.
  - `ScriptedUserAction` exposes script hooks for names, visibility, and performability, but not a script-side priority/order override. For the HST HQ arsenal, keep the custom loadout action before the HQ menu action in `Prefabs/Objects/HST/HST_HQArsenal.et`, disable inherited stock arsenal actions through `HST_HQArsenalActionFilterComponent`, then assert the first selectable runtime action through `ActionsManagerComponent.GetActionsList()`.
  - Treat the campaign-debug assertion `hq.arsenal.loadout_editor_first` plus the `h-istasi HQ arsenal actions` evidence row as the proof that the Loadout Editor is the first selectable arsenal option. If the row reports a different first selectable action, the prefab/action-filter order is wrong even if the editor can still be opened manually.

- Layout defaults should match the script's first meaningful state.
  - For mode-driven screens, mark always-on chrome explicitly visible and mark inactive mode panels hidden in the layout.
  - Let script show the active mode panel during population instead of relying on a cleanup pass to hide every inactive panel after creation.
  - Current example: `HST_LoadoutEditor.layout` keeps core chrome visible and starts candidate/storage/save panels hidden.

## Coordinates And DPI

- Workspace raw size and layout size are different under UI scaling.
  - Example seen in testing: raw `2560x1440`, layout `1920x1080`.
  - Use `HST_UIWorkspaceMetrics.GetRawWorkspaceSize` only for raw pixel needs.
  - Use `HST_UIWorkspaceMetrics.GetLayoutSize` for layout coordinates.
  - Use `HST_UIWorkspaceMetrics.LayoutToRawPx` / `RawToLayoutPx` for conversions.

- Keep direct `workspace.DPIUnscale` / `workspace.DPIScale` calls inside the metrics helper.
  - Map projection overlays still receive native raw screen coordinates from `SCR_MapEntity.WorldToScreen`, but should route those values through `HST_UIWorkspaceMetrics.RawToLayoutPx`.

- `SCR_MapEntity.WorldToScreen` gives native raw screen coordinates.
  - Convert projected x/y with `HST_UIWorkspaceMetrics.RawToLayoutPx` before `FrameSlot.SetPos`.
  - For world-radius circles, project the center and a world-space edge, derive the raw pixel radius, then convert the final raw size with `HST_UIWorkspaceMetrics.RawToLayoutPx`.

- `SCR_MapEntity.ScreenToWorld` must wait for map readiness.
  - Do not use it before the map is open, `OnMapOpenComplete` has fired, the map has advanced a few frames, and the map widget has non-zero size/valid zoom.
  - This avoids division-by-zero and invalid setup selections.

## Map UI

- `SCR_MapEntity.SetupMapConfig` caches the active map config by `EMapEntityMode`, not by config resource path.
  - If a custom setup map uses `EMapEntityMode.FULLSCREEN`, the next normal fullscreen map can reuse the setup config and keep the setup-only UI component stack.
  - Use a distinct map mode for custom setup maps, such as `PLAIN`, and keep the normal player map on `FULLSCREEN`.
  - Current example: `HST_SetupHQMap.conf` / `HST_SetupMapComponent` use `PLAIN`; `HST_GameplayMap.conf` inherits vanilla fullscreen behavior for the normal map.

- Setup map config and gameplay map config must stay separate.
  - Setup can use a minimal map config with setup selection behavior and optional setup overlays.
  - Gameplay should inherit/preserve vanilla `MapFullscreen.conf` tools and marker UI.
  - Setup-only components must not pollute normal map behavior after HQ placement.

- Gameplay command targeting should open the normal map, not the setup map.
  - Use `MenuManager.OpenMenu(ChimeraMenuPreset.MapMenu)` for player-facing
    command targeting so the player sees the same marker/tool stack as a normal
    map open.
  - Bind `SCR_MapEntity.GetOnSelection()` after opening, wait for
    `GetOnMapOpenComplete()`, convert clicks with `ScreenToWorld()`, and manage
    selection state through `SCR_MapCursorModule.ToggleLocationSelection()`.
  - A confirmation modal over the normal map should use
    `SCR_MapCursorModule.HandleDialog(true/false)` and temporarily disable
    location selection instead of activating extra dialog input contexts.
  - If an action requires map targeting, gate the visible action by
    `SCR_GadgetManagerComponent.GetGadgetByType(EGadgetType.MAP)` and revalidate
    the map on the server before executing the command. The client menu state is
    presentation, not authority.
  - Current examples: `HST_CommandMenuComponent.BeginCommandMapTargetSelection()`
    and `HST_CampaignCoordinatorComponent.PlayerHasMapInInventory()`.

- Campaign markers should use native map marker systems.
  - Let `SCR_MapMarkerManagerComponent` handle projection and pan/zoom.
  - Keep zone radius circles as optional `SCR_MapUIBaseComponent` overlays, not campaign markers.
  - Player-requested resistance support has two marker lifetimes: the request marker follows queued/active call-in state, and the live group marker follows the spawned active group. `features.trackResistanceSupportGroupsOnMap` defaults to enabled; when it is on, keep the live marker linked to the active group id until the group is terminal, killed, folded, or despawned. Do not hide the live marker just because the request has reached resolved/arrived status.

- Dynamic player markers need a marker config entry before `InsertDynamicMarker` can work.
  - `SCR_MapMarkerManagerComponent.InsertDynamicMarker(type, entity, configId)` resolves `type` through the active `SCR_MapMarkerConfig`; stock `CampaignMapMarkerConfig.conf` does not expose every dynamic marker type.
  - The custom config must be the marker manager config on every gameplay/dev world layer that should show the marker. Having `Configs/Map/HST_PlayerMapMarkerConfig.conf` in the addon is not enough if the active layer still points at a config without `SCR_EMapMarkerType.HST_PLAYER`.
  - If a world layer still points at the vanilla marker config, patch the active `SCR_MapMarkerManagerComponent` before reconciliation with `EnsureHSTMarkerConfig`, loading `Configs/Map/HST_PlayerMapMarkerConfig.conf` and initializing its dynamic entries. The service should retry and log config readiness instead of silently inserting an unconfigured marker type.
  - Server-side config repair is not enough for dynamic marker rendering. `SCR_MapMarkerEntity.OnCreateMarker()` resolves its widget entry on the client from the local marker manager config, so custom player marker entities must also ensure `HST_PlayerMapMarkerConfig.conf` before vanilla widget creation asks for `SCR_EMapMarkerType.HST_PLAYER`.
  - Inherit the custom config from the known `CampaignMapMarkerConfig.conf` parent and copy required HQC squad entries into the HST config. Do not depend on an unverified parent GUID for `CampaignHQCMapMarkerConfig.conf`; a bad parent resource ID makes the whole marker config fail before the HST entry can be used.
  - Do not use `PLACED_CUSTOM` for tracked entities; it is a static marker type. For h-istasi player tracking, the world layers point the marker manager at `Configs/Map/HST_PlayerMapMarkerConfig.conf`, which inherits the campaign marker config and adds `HST_PlayerMapMarkerEntry`.
  - Do not reuse `SCR_EMapMarkerType.DYNAMIC_EXAMPLE` for custom dynamic markers. `SCR_MapMarkerConfig.GetMarkerEntryConfigByType` returns the first matching entry, and the parent map marker config already defines the stock example entry. Add a dedicated modded enum value such as `SCR_EMapMarkerType.HST_PLAYER`.
  - The stock `SCR_MapMarkerEntryDynamicExample` registers its own spawn/death handlers. If h-istasi owns marker lifecycle through a service/reconciler, use a custom `SCR_MapMarkerEntryDynamic` subclass, call `super.InitServerLogic()` to bind the manager, and do not register stock events.
  - Custom marker entry classes used from `.conf` need the same `[BaseContainerProps(), SCR_MapMarkerTitle()]` attributes as stock marker entries; without them the config entry can fail to instantiate even though the script class exists.
  - Runtime symptom: `Unknown class 'HST_PlayerMapMarkerEntry'` while loading `HST_PlayerMapMarkerConfig.conf` means the config can load but the marker entry script class was not registered/compiled for that run, so `InsertDynamicMarker` will not have a usable `HST_PLAYER` entry.
  - Validate both the imageset resource and the quad name. `{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset` has `circle` but not `dot`; using a missing quad can leave an otherwise created marker visually blank.
  - Current h-istasi player marker art uses the `whisper` quad from `{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset`; keep the dynamic marker layout passive with `Ignore Cursor` so it cannot steal map interaction.
  - `SCR_MapMarkerEntity.SetText()` is not replicated. If server-created dynamic markers need per-player labels, pass a stable value through replicated marker data such as `configId` and resolve the client-side label in `InitClientSettingsDynamic`. Replicated marker fields can arrive around widget creation, so reapply labels briefly from the callqueue if the first pass only has fallback text.
  - Reconciler dynamic cleanup must remove tracked domain ids even if the stored `SCR_MapMarkerEntity` pointer is already null. A stale dynamic handle can poison tracked counts and prevent player marker cleanup/recreate from converging.
  - Matching tracked dynamic handle counts are not enough to skip player marker reconciliation. Check that tracked `SCR_MapMarkerEntity` references are still present in `SCR_MapMarkerManagerComponent.GetDynamicMarkers()`, and recreate handles that have unregistered from the native manager.
  - Player markers use a separate reconciler from campaign/HQ/zone markers. Include `HST_PlayerMapMarkerService.BuildRuntimeReport()` in native marker admin reports so tests can see desired player records, tracked handles, live native handles, native dynamic marker count, marker-config entry readiness, and last reconcile failures in one place.
  - Admin native-marker purge/recovery commands must clear both the campaign marker reconciler and the separate player marker reconciler. If player markers are enabled, queue a player marker refresh after purge so live player handles are recreated from current controlled entities.
  - `SCR_PlayerNamesFilterCache` can lag marker widget creation and may return shortened display text. Prefer `PlayerManager.GetPlayerName(playerId)` when it is available, then fall back to the filter cache, and retry for a few seconds before accepting a generic `Player X` label.
  - If `HST_PlayerMapMarkerService` logs `native reconcile failed`, first verify the active world layer `SCR_MapMarkerManagerComponent.m_sMarkerCfgPath`, the custom marker entry config, and the dedicated `SCR_EMapMarkerType.HST_PLAYER` enum.
  - For custom dynamic visuals with client-resolved image/text, do not rely on `SCR_MapMarkerEntryDynamic.InitClientSettingsDynamic()` because base marker text/imageset are not replicated for server-created dynamic markers. Set image/color/text directly in the custom entry and force a visible alpha on faction colors.
  - Dynamic marker config entries must name a layout with `SCR_MapMarkerDynamicWComponent` or a subclass. The default `SCR_MapMarkerEntryConfig` layout is `MapMarkerBase.layout`, and `MapMarkerSquadMember.layout` uses `SCR_MapMarkerSquadMemberComponent`; both lack the dynamic handler that `SCR_MapMarkerEntity.OnCreateMarker()` dereferences on map open.
  - Native dynamic markers follow movement through their target entity. `SCR_MapMarkerEntity.SetTarget()` enables frame updates, and authority-side `EOnFrame()` copies `m_Target.GetOrigin()` into the replicated marker position, so player markers should not be recreated just to follow walking/driving movement.
  - The current `whisper` player-marker icon needs a forward-facing rotation offset of `-50` degrees before applying player yaw to line the icon up with the in-game facing direction.
  - Server-created dynamic marker prefabs need exactly one valid replication component. For h-istasi player markers, prefer a standalone `SCR_MapMarkerEntity` prefab with one local non-spatial/non-streamable `RplComponent`, matching the native squad marker shape. Do not inherit `MapMarkerEntityBase.et` and then add another `RplComponent`; the duplicate-component warning (`SCR_MapMarkerEntity component RplComponent cannot be combined with component RplComponent`) can leave `FindComponent(BaseRplComponent)` null during marker init and make player markers reconcile but fail to render.
  - Vanilla `SCR_MapMarkerEntity.EOnInit()` assumes `FindComponent(BaseRplComponent)` succeeds. If a dynamic marker prefab/resource state can produce a marker entity without a resolved RPL component, guard the modded init path before calling `IsOwner()` or `InsertDynamicMarker` can throw `NULL pointer to instance. Variable 'rplComp'`. Keep the marker entity active when RPL is temporarily unavailable; `SetTarget()` only sets the frame event mask, while active frame updates are what copy the target position into the replicated marker position.
  - Player marker facing rotation lives in the marker widget component, not the marker service. `HST_PlayerMapMarkerDynamicWComponent` rotates only `MarkerIcon` from the player entity's map yaw, leaving `MarkerText` upright and avoiding extra server marker churn. The `whisper` icon art points roughly 50 degrees off its widget zero after in-game alignment, so apply the icon forward offset when converting yaw to widget rotation.
  - Current working player-marker layout: `{6985327711306214}UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout`.
  - Custom player marker entries that set a name must also call `SetTextVisible(true)`.
- Player markers should publish in gameplay-bearing campaign phases (`HST_CAMPAIGN_ACTIVE`, `HST_CAMPAIGN_WON`, and `HST_CAMPAIGN_LOST`) but not during setup/bootstrap. Campaign-debug phase 24 intentionally leaves the state terminal at the end of a run, and clearing player markers just because the phase is no longer active makes the post-run map look broken even though gameplay state still exists.
  - For faction-colored player markers, resolve `SCR_FactionManager.SGetPlayerFaction(playerId)` client-side and fall back to the FIA faction color before using a hardcoded color.
  - Player marker reconcile signatures must include the resolved player/entity faction key, not just player id, target entity, and name. Faction assignment can arrive after the marker is created, and skipping reconciliation on an unchanged entity can leave the native marker with stale stream rules.
  - Do not hard-code the native marker stream faction to FIA. Use `SCR_FactionManager.SGetPlayerFaction(playerId)` first, then the controlled entity's `FactionAffiliationComponent`, and leave the stream key empty if neither is available so a bad fallback does not hide the player's own marker.
  - HST player markers should call `SCR_MapMarkerEntity.SetFaction()` once a real player/entity faction is known so native map faction visibility can include them. Keep the widget icon/text color client-resolved from the same player faction, and avoid hardcoded fallback stream factions that can hide hosted-server markers.
  - `SCR_MapMarkersUI.CreateDynamicMarkers()` can call `SCR_MapMarkerEntity.OnCreateMarker()` before the map frame is actually usable. A log before `super.OnCreateMarker()` only proves creation was requested; the reliable success signal is a non-null marker root/widget component or the custom marker entry's `InitClientSettingsDynamic()` log.
  - Gameplay-map open repair should retry until HST dynamic player marker entities exist and their widgets are created. Treat `0` dynamic HST player markers as not-ready for a few retries, because the replicated marker entity can arrive after `OnMapOpenComplete`.
  - If a cached dynamic marker widget root is detached from the current map hierarchy, clear the root and widget component before rebuilding. Native dynamic marker deletion/removal paths can leave a stale reference even though the visible map root has been destroyed.
  - Current examples: `HST_PlayerMapMarkerService`, `HST_PlayerMapMarkerEntry`, `HST_PlayerMapMarkerConfig.conf`.

- Map overlay widgets must be passive.
  - Create overlay primitives with `IGNORE_CURSOR` / `NOFOCUS`.
  - Redraw only when content or viewport state changes enough to matter.
  - Do not redraw on every frame while idle.

- Setup-map zone labels are generated by `HST_MapZoneOverlayUIComponent`, not by `HST_SetupHQMap.layout`.
  - Change location-name text color in `DrawZone()` / `CreateZoneLabel()`.
  - When labels are created dynamically, set color on the cast `TextWidget` with `SetColorInt()` as well as any generic widget color; otherwise text can render with the layout/default color even though the widget exists.
  - Black setup location labels need a light outline/shadow halo because the underlying map texture varies; candidate labels can keep their candidate color.

- Keep map overlay redraw ownership in the map UI component.
  - Setup/map-flow components should publish content changes, such as setup zones or the temporary HQ candidate marker.
  - The `SCR_MapUIBaseComponent` overlay should own projection, revision checks, pan/zoom thresholds, and widget reuse.
  - Avoid a second dirty/redraw loop in the flow component; duplicate redraw ownership makes readiness and input bugs harder to isolate.

## Input And Widget Events

- Custom input actions need an active `ActionContext`, not just an `Action`.
  - `InputBinding.CreateUserBinding()` and `AddActionListener()` can appear successful for an action defined only under top-level `Actions`, but `GetActionTriggered()` and the listener may never fire because no active context owns the action.
  - Put mod-owned actions inside a custom `ActionContext`, register the config as a custom input config, and activate that context before polling or listening.
  - Current example: `Configs/HST/Input/HST_Input.conf` defines `HST_CommandMenuContext` with `HST_CommandMenu`, and `HST_CommandMenuComponent` activates that context before polling the `I` key command-menu action.
  - If a key is covered by both raw `Debug.KeyState()` fallback polling and an action-manager binding, consume the first edge for the frame before polling the second path. Otherwise one physical key press can produce duplicate open/close or debounce-refusal evidence that makes the input path look broken.
  - Log action edges, raw key edges, and UI-root refusal state while diagnosing input. A log that only says a component registered its listener does not prove the key path fired.
  - Enfusion string-formatting of bools in debug reports may appear as `1`/`0`, not `true`/`false`. Server-side parsers for owner-client UI proof reports must accept both forms, otherwise `menuOpen 1` and `root 1` can be misclassified as a closed menu.
  - Rendered UI proof lists should include only widgets that are expected to be visible in the normal layout. Do not require hidden-by-design labels or optional empty panels, such as a blank header tab title or empty activity feed rows, as full-menu readiness gates.

- Full-screen modal UIs that close on Escape need to consume both menu actions bound to Escape.
  - Base Reforger configs bind `KC_ESCAPE` to `MenuBack` and `MenuOpen`; listening only to `MenuBack` can close the mod UI and still let the native pause menu open from the same keypress.
  - HST workspace-widget overlays are not native `MenuBase` screens. The global Reforger `MenuOpen` path can therefore decide that no menu is open and spawn `PauseMenu` after the HST overlay closes.
  - While the modal is topmost, listen for `MenuBack`/`MenuOpen`, poll raw `KC_ESCAPE` as a fallback, call `Debug.ClearKey(KeyCode.KC_ESCAPE)`, reset the stock `MenuBack`/`MenuOpen` input actions, and schedule pause-menu dismiss retries for a short window while the original Escape press is still down.
  - `PauseMenuUI.m_OnPauseMenuOpened` is useful for workspace-widget overlays that are not visible to `MenuManager` as a native top menu. Register the guard only while the overlay is open, keep it through the short Escape-dismiss retry window, call `MenuManager.CloseMenuByPreset(ChimeraMenuPreset.PauseMenu)` or close the found pause menu, then unregister when the overlay and pending-dismiss window are both gone.
  - Current example: `HST_LoadoutEditorComponent` routes Escape/gamepad menu-back through the same editor back stack as the on-screen Back button. Escape backs out one submenu layer at a time and closes the editor only when already at the root, while still consuming the native pause-menu input.

- Real controls should be widget-driven.
  - Give interactive widgets stable `UserID`s and attach a `ScriptedWidgetEventHandler`.
  - Avoid raw coordinate hit testing for visible buttons.
  - When a render path programmatically syncs an `EditBoxWidget` with `SetText`, guard the paired `OnChange` handler so the sync cannot re-enter the render path. Attach the handler after the sync where practical, but still use an explicit guard because the reused widget may already have a handler.

- Some interactions may arrive through both `OnClick` and `OnMouseButtonUp`.
  - Prefer a single activation owner for normal buttons, usually `OnClick`.
  - Use duplicate-activation guards only where a widget action can unavoidably be triggered by both callbacks in the same frame.
  - Pass the mouse button through both event paths and key the guard by widget id, button, and frame serial so right-click/left-click actions remain distinct.
  - Current examples for guards include `HST_SetupMapComponent`, `HST_CommandMenuComponent`, and `HST_MissionClientComponent`; `HST_LoadoutEditorComponent` keeps normal button commands on `OnClick` and uses mouse-up only for preview drag release.

- Avoid synchronously deleting and rebuilding a newly added control subtree from that same control's callback.
  - Existing long-lived menus may tolerate direct `RenderEditor()` calls from established row buttons, but new filter/sort/search controls are safer when they mutate state and queue a zero-delay callqueue render.
  - This keeps the handler return path from walking widgets that were just destroyed by the render.
  - Current example: `HST_LoadoutEditorComponent.QueueStorageBrowserRender()` for storage filter/sort/search controls.
  - The storage browser tab list is the authority for server-side candidate categories and client search. When a tab is removed, narrow both `HST_LoadoutEditorService.IsStorageBrowserCandidateCategory()` and client search/category helpers so hidden wearable/container categories cannot be returned through search.
  - `RenderTargetWidget.SetClearColor(true, color)` is needed for loadout preview background presets because the render target can remain opaque over the UI backdrop; changing only the backdrop or a sky-sphere material may not affect the visible clear/background color.
  - Keep storage search scroll state separate from the normal storage candidate grid scroll. Search query/results intentionally persist while the editor stays open, but search rerenders should restore `m_fStorageSearchScrollY` rather than inheriting the category grid's `m_fStorageCandidateScrollY`.

- Modal state belongs in one coordinator.
  - `HST_UIRootService` tracks current screen, modal screen, blocking behavior, and topmost owner.
  - Repeated identical `RequestOpen` calls should be idempotent; otherwise refresh loops create noisy revision churn.
  - Use distinct screen modes for distinct modal families. Action confirmations register as `ACTION_DIALOG`; mission report/details dialogs register as `MISSION_DIALOG`.

- Screen input and modal input need separate ownership gates.
  - A full-screen UI should only process keyboard, preview drag, and widget clicks when `HST_UIRootService.CanHandleScreenInput(mode, owner)` matches that screen and no modal is above it.
  - A modal should process its buttons through `CanHandleModalInput(mode, owner)` instead of checking the underlying screen's open flag.
  - This matters when a contextual command opens only a confirmation modal while the command menu shell is closed; the modal still owns Yes/No input, but the closed screen should not receive normal menu navigation.
  - Current examples: command action dialogs, mission report close, and loadout editor preview drag/tab/back input.

- Shared confirmation rules need to cover both row clicks and contextual commands.
  - Command-menu row activation and contextual user actions can reach the same campaign mutation through different methods.
  - Put the confirmation decision in the destination UI component, not in a specific row factory, so actions such as HQ relocation and mission-objective completion cannot bypass the shared modal when triggered from world-space user actions.
  - If a contextual action opens only the confirmation modal while the command menu is closed, confirmed/cancelled callbacks must not call the full command-menu render path unless the menu is actually open.
  - Current example: `HST_CommandMenuComponent.ShouldConfirmAction`, `RunCommandFromContext`, and `RequestConfirmedAction`.

- A modal over a native map needs cursor handling split between the UI root and the map selection mode.
  - Do not activate `DialogContext` / `InteractableDialogContext` over the map just to make buttons clickable; those contexts can create an extra OS-style pointer over the map cursor.
  - Disable the current map selection mode directly, for example `ToggleLocationSelection(false)`, while the modal is waiting for an answer.
  - Use `SCR_MapCursorModule.HandleDialog(true)` / `HandleDialog(false)` for map-owned dialog state so the native map cursor is allowed to travel above the modal and selection/drag cursor modes are suppressed.
  - The native custom map cursor lives in its own workspace cursor layout at z-order `10`. Keep setup-map modal chrome above the map root but below that cursor band, for example modal root/dimmer/dialog/buttons/text in `1..9`, when the map cursor must remain visible over confirmation buttons.
  - Do not force `WidgetManager.SetCursor(0)` over an active map cursor; the native map cursor already hides the real cursor and forcing it back creates a second pointer.
  - Avoid modal-owned cursor proxy widgets over native map dialogs. They are easy to layer above the dialog, but they create a third visible cursor and drift from the engine cursor lifecycle.
  - For normal gameplay-map target confirmations, a fixed selected-target indicator is acceptable when it is not an engine cursor proxy: create it as a passive workspace overlay above `Z_ACTION_DIALOG`, set `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` on every widget, convert `SCR_MapEntity.WorldToScreen()` raw coordinates with `HST_UIWorkspaceMetrics.RawToLayoutPx()`, and clear it whenever the dialog closes or the player chooses another point.
  - Current examples: `HST_SetupMapComponent` and `HST_CommandMenuComponent`.

- Notifications should not participate in blocking input.
  - Keep toast roots and children cursor-ignored and no-focus.
  - Track notification depth separately from current/modal screens.

## Enforce Script Practicalities

- `string.Format` placeholders are limited to `%1` through `%9`.
  - Do not use `%10` or higher in diagnostics or reports; split the report into multiple `string.Format` calls or append the remaining values with string concatenation.

- Enforce can reject long boolean expressions with `Formula too complex`.
  - Split large assertion expectations into guarded blocks and smaller named
    booleans, then combine them over multiple assignments.
  - Current example: the physical-response save-roundtrip assertion in
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugPhysicalResponseFoldbackCase()`.

- `reference` is an Enforce keyword, not a safe local variable or parameter name.
  - Runtime symptom: Workbench reports `Broken expression (missing ';'?)` on a declaration such as `string reference;`.
  - Use a normal identifier such as `contextText`, `referenceText`, or `currentText` instead.

- Enforce compile validation can reject very wide helper signatures with `Maximum arguments count 16 exceeded`.
  - Campaign debug case/probe helpers should pass a small runtime context object once they need many observed fields. Current example: `HST_CampaignDebugSupportProbeContext` carries support ETA/status/physicalization observations between the support runtime probe and typed assertion builder.
  - Keep assertion calls flat in large debug builders: compute `actual` strings and status booleans before `AddCampaignDebugAssertion(...)` instead of nesting several helper calls inside the assertion call.
  - Avoid reusing generic parameter names such as `request` across adjacent support helpers when fixing wide signatures; a previous support probe refactor produced follow-on `Multiple declaration of variable` errors until the shared state moved into a context object and parameters were renamed to `supportRequest`.

- Enforce parser diagnostics can blame the first statement inside a helper when the method signature is the real problem.
  - Current observed case: a campaign-debug marker-audit helper with six `out` parameters reported `Broken expression (missing ';'?)` on the following `foreach` line during Workbench Game script compilation.
  - Keep detailed debug counters in a context object or component fields and use at most a small number of `out` parameters on hot helpers.
  - A valid-looking chained helper branch can also report the error on the next plain `return`. If a marker/debug audit helper combines category checks with direct `return service.FindX(...) != null` expressions, prefer explicit typed locals plus simple branch returns; this keeps Workbench's parser from blaming the following statement.

- Avoid redundant `Cast(...)` calls when the API already returns the requested base type.
  - Workbench reports this as `No need to use 'Cast' for up-casting`.
  - Current example: `AIAgent.GetMovementComponent()` can be returned directly from a method typed as `AIBaseMovementComponent`.

- Campaign debug physical probes should recover or block on player liveness before real interactions.
  - `SCR_DamageManagerComponent.FullHeal()` is useful only for a non-destroyed controlled entity; destroyed players need the normal h-istasi respawn sweep.
  - Put liveness guards at the teleport/interaction boundary so later cargo, captive, or area probes do not report misleading `player is not alive` mission-action failures caused by an earlier physical probe.
  - Do not pre-block render-bubble, civilian population, or similar physical probes solely because the current controlled entity is not living. If bootstrap has not already marked physical tests blocked, call the shared campaign-debug living-player recovery path first, then record `BLOCKED` only if recovery still cannot produce a living entity.

- Vehicle source capability metadata is persisted state from campaign schema 19 onward.
  - Use prefab-based `HST_VehicleCapabilityPolicy` backfill for pre-schema-19 saves, new captures, or direct world scans; do not reapply it during current-schema save restore because it can erase explicit source flags.
  - When redeploying a garage vehicle, copy its persisted source kind and ammo/repair/fuel flags onto the runtime record and preserve those copied values.

- Prefer explicit boolean checks for object references when returning a bool.
  - `if (widget) return true;` is clearer and safer than returning widget-reference expressions from `bool` methods.

- `CallLater` supports delayed UI repair/diagnostic passes and can accept arguments.
  - Existing examples with arguments include notification dismissal generation checks and preview entity finalization.

- Reused layout roots need container cleanup, not just tracked-widget cleanup.
  - If generated rows are created into layout-owned containers and the root is reused, remove tracked dynamic widgets and then remove all children from the dynamic containers before repopulating.
  - Hidden or parent-hidden generated widgets can survive a mode switch if cleanup only clears visible children. Remove tracked dynamic widgets regardless of current visibility before the container sweep.
  - Reused static widgets should not call `AddHandler(m_WidgetHandler)` on every render. Route editor-owned bindings through an idempotent helper that checks `FindHandler(EditorWidgetHandlerType)` before adding the handler, or repeated renders can stack callbacks on filter/sort/close buttons.
  - `FrameWidgetSlot` does not accept `HorizontalAlign` / `VerticalAlign`. Keep those keys inside `AlignableSlot` or `OverlayWidgetSlot`; if the parser reports unknown alignment keywords at layout load time, inspect the nearest generated `FrameWidgetSlot` first.
  - When script populates text into layout-owned controls, set a known font in the common text setter. Newly added `TextWidget` labels without `FontProperties` can otherwise appear as blank clickable buttons even though the widgets exist.
  - Do not route normal button activation through both `OnClick` and `OnMouseButtonUp`. Use mouse-up only for drag state that explicitly consumes the event; otherwise return false and let `OnClick` perform the command once.
  - For layout-owned buttons whose label is a sibling `TextWidget`, validate and log both the button binding and the label widget summary when debugging blank labels. A visible/clickable button with a missing, hidden, zero-sized, or behind-parent label otherwise looks like a script click problem in the runtime log.

- Keep live inventory payloads single-owner.
  - If a server payload emits real storage contents as storage-node children, do not also re-emit those same entries as synthetic top-level inventory rows.
  - A simple guard is to treat slots with `m_sSlotKind == "storage_item"` or a non-empty parent slot id as nested storage rows only.
  - Clearing only an array of widgets misses rows that were not inserted into that array and causes tabs/lists to accumulate stale entries.
  - Hidden layout regions may not be reliably found by a global cleanup pass, depending on visibility and resolution timing.
  - Clear the active dynamic container again after the region has been resolved and shown, then populate it.
  - Treat the selected loadout edit target as a single resolved context, not as separate slot-only and node-only states. A layout row may leave only a selected slot id while candidate payloads and remove commands still need the live node id.
  - Invalidate compatible-item candidate payloads after loadout or storage mutations. Arsenal counts, storage capacity, and compatibility can change immediately after inserting/removing an item, so a cached empty candidate result can become stale.
  - Storage candidate troubleshooting needs separate counts for recovered arsenal items, slot/category matches, and live inventory compatibility matches. A category can have recovered items and still return an empty visible panel because the selected storage has no live insert target, no free capacity, or no compatible slots after the last insertion.
  - Storage candidate payloads should include matching recovered arsenal items even when the selected storage cannot currently accept them, with the `compatible` field carrying the fit result. Keep non-storage candidate lists server-filtered to compatible items only.
  - Magazine ammo usability can be computed independently from storage fit. This lets the UI show an "ammo usable" filter even when the item cannot fit in the selected container.
  - Storage capacity display should mirror `SCR_InventoryStorageBaseUI`: direct inventory storages can use `GetOccupiedSpace()` / `GetMaxVolumeCapacity()`, but `ClothNodeStorageComponent` capacity must be summed from owned `SCR_UniversalInventoryStorageComponent` instances. The cloth-node component itself can report misleading full/empty percentages.
  - Storage browser filter/sort state belongs in the local visual settings file, not the campaign state. Defaults should be fit-only and A-Z, with server authority still enforced by `add_storage_item`.
  - The storage ammo filter is tab-contextual. Keep its saved bit when leaving the ammunition tab, but only apply and show it active while `m_sSelectedCategory == "magazine"` so other tabs do not appear filtered by ammo.
  - Name sort direction and count sort direction are separate settings. When count sorting is selected, use count as the primary order and A-Z/Z-A as the tie-breaker; this lets the UI show `A-Z`/`Z-A` and `INF-1`/`1-INF` together.
  - Non-fitting storage candidates should keep their arsenal count badge. Use a light red row highlight for the fit failure and let the server-authoritative `add_storage_item` path reject clicks that still do not fit.
  - Structural pouch/holster/bandolier/carrier/scabbard/sheath/e-tool carrier or case-style storage attachments should not be emitted as addable storage-browser candidates. Check prefab, display, and short display names before spawning compatibility probes, because Reforger item paths and localized display names can expose different structural-container clues. These attachments can be child containers on looted webbing, but the player should add/remove the parent webbing or magazines inside those child pouches, not the pouch prefab itself.
  - Block structural inventory containers at deposit time and purge them from saved arsenal state. The current common bucket is non-loadout-clothing entities with structural attachment/cargo storage plus display/prefab tokens such as personal belongings, pouch, holster, bandolier/bandoleer, scabbard, sheath, compass case, suspenders, and e-tool/entrenching-tool carriers. Do not block real wearable backpacks/field packs or loadout cloth components just because they have cargo storage.
  - Structural wearable/storage prefab tokens are inconsistent. Treat `fieldpack`, `field_pack`, `field pack`, `ruck*`, and ALICE pack names as backpack/wearable storage aliases, while blocking `pouch`, e-tool `carrier`/`case`/`holder`/`cover` qualifiers, and similar structural child containers from arsenal storage candidates. Do not block the actual equippable entrenching tool just because its name contains `etool` or `entrenching tool`.
  - Run structural child-container token checks before source-category fallback in the loadout editor. If an arsenal entry arrives as `backpack`, `vest`, or generic equipment but its prefab/display says `buttpack`, `pouch`, `scabbard`, `compass case`, or e-tool carrier/case/cover, classify it as utility and let the structural filter block it instead of letting the source category resurrect it as wearable gear.
  - Broad `carrier` text is not enough by itself to block an item; real wearable armor carriers exist. Treat carrier as structural only with stronger context such as entrenching-tool, compass, radio, pouch, or case-style tokens.
  - Area-loot classification needs the same wearable/container resolver as the loadout editor. Worn BDU blouses, ALICE webbing, vests, pants, and backpacks can have cargo storage and still be real loot; classify them as loadout clothing before the structural cargo-storage probe. Explicit structural tokens such as pouches, suspenders, compass cases, scabbards, and e-tool carriers still win before wearable exemptions.
  - Storage content rows should gather from every insert-capable storage under the selected worn container, then filter out structural container shell entities before grouping rows. This lets magazines/flashlights inserted into webbing child pouches show in the bottom-left contents without showing the pouch/carrier as an add/remove item.
  - For worn webbing such as ALICE, insert-capable storage means the real cargo/deposit storages reached by recursing into child pouches, not the parent `ClothNodeStorageComponent` or structural attachment slots themselves. Including structural attachment storages in the insert/capacity list can inflate percentages, target pouch shells, and reject otherwise valid cargo inconsistently.
  - Treat paper map item prefabs as arsenal-equivalent across factions. `PaperMap_01_folded_US`, `PaperMap_01_folded_USSR`, `PaperMap_01_folded_FIA`, and the generic folded map should share arsenal lookup, issued-loadout lookup, saved-loadout cost grouping, and refund/withdraw accounting.
  - Keep client search filtering and server candidate/deposit filtering on the same shared token helper (`HST_ArsenalItemFilter.HasBlockedStructuralContainerToken`). A separate client-only list can miss cases such as compass cases or suspenders and show entries the server will reject.
  - Explicit blocked structural tokens must run before any broad wearable exemption. After pouch/carrier/case-style tokens are rejected, `BaseLoadoutClothComponent` is a valid wearable signal because some real clothing/backpack/webbing items arrive from loot classification as generic equipment or utility.
  - Bandages/field dressings use medicine-prefab paths and may not include the word `Bandage` in the prefab. Classify `Medicine`, `FieldDressing`, `FirstAid`, `Bandage`, `Morphine`, `Tourniquet`, and `Medkit` tokens as `medical` before falling back to generic equipment.
  - Storage-browser candidate preview keys must ignore `live_storage_item_` draft slots. Changing the contents of the selected container should refresh inventory lists and capacity bars, but it should not rebuild/refocus the rendered entity unless the displayed container/gear actually changes.
  - Wrap-layout storage grids need children whose root slot is `WrapLayoutWidgetSlot`, and the tile's inner `SizeLayout` must not horizontally fill the parent. Reusing a vertical-row layout or a fill-aligned size wrapper can collapse the right-hand storage browser to a single column even when a fixed tile width is set from script.
  - If a wrap-grid child is a clickable `ButtonWidget`, also keep a `SizeLayoutWidget` child with explicit width/height and enforce those overrides when creating rows. Otherwise the button root can stretch to the scroll width and visually behave like a single-column list even under a `WrapLayoutWidget`.
  - Do not use `LayoutSlot` manipulation helpers on a widget whose root slot is `WrapLayoutWidgetSlot`; the engine logs an invalid-slot error and click handlers can crash. Resize storage-browser tiles through the inner `SizeLayoutWidget` width/min/max overrides instead.
  - Two-column storage browser tiles need to derive width from the active right pane, not a single global constant. Reserve room for panel margins, tile padding, and the scrollbar, then clamp to a practical min/max so narrow panes keep two columns and wide panes use the available text area.
  - Worn clothing swapped through live inventory can expose useful cargo through `BaseInventoryStorageComponent.GetOwnedStorages()` even when the direct clothing storage has no insertable slots. Discover owned cargo/deposit storages before skipping zero-slot parent storage components, or BDU blouses, ALICE webbing, and similar swapped clothing can disappear from the storage tab while still showing capacity in the vanilla inventory UI.
  - Some worn clothing/webbing cargo storages live on child entities under the equipped item rather than on the equipped entity's own storage component or owned-storage list. Bounded child-entity recursion is needed when collecting cargo/deposit storages for the storage tab.
  - Some swapped clothing storage is volume-only from the loadout editor's perspective. Treat a storage with `GetMaxVolumeCapacity() > 0` as capacity-bearing even if `GetSlotsCount() == 0`, then recurse owned/deposit storages for actual insert targets and contents.
  - Empty live loadout slot compatibility sometimes needs a category fallback before exact `LoadoutAreaType` equality. Looted backpacks/field packs can be correctly classified as `backpack` in the arsenal while the native slot-area type comparison still rejects them, so validate the live node category first for loadout clothing categories.
  - Parent worn-slot replacement candidates must pass prefab/display-derived wearable category filtering before trusting native live storage compatibility. Some accessory items such as canteens are `BaseLoadoutClothComponent` child-slot items and may pass `CanReplaceItem`, but they are not valid Jacket/Headgear/Pants/Boots/Backpack parent-slot replacements.
  - Keep armored vests and load-bearing webbing as distinct editor categories. PASGT/armor vests should label as `Armored Vest`; ALICE/LBE/chest-rig/grenadier-style webbing should label as `Chest Rig`. Run backpack alias detection before webbing detection so ALICE packs/field packs remain backpack candidates.

- Loadout editor saved loadouts:
  - Fixed save slots are server-owned campaign state and profile persistence. Passing `slot_N` to `loadout_save` selects the slot; it must not overwrite an already renamed display name unless the slot was empty or an explicit rename command was sent.
  - Use `JsonSaveContext.SaveToString()` and `JsonLoadContext.LoadFromString()` with `SCR_PlayerArsenalLoadout.ReadLoadoutString/ApplyLoadoutString`. Obsolete `SCR_Json*` helpers can compile with warnings or fail to round-trip current native loadout data.
  - Serialized native saved loadouts include attachment and child-slot metadata that may not be present as h-istasi recovered arsenal entries. For serialized slots, skip attachment metadata and other missing-arsenal rows in the cost ledger instead of failing before `SCR_PlayerArsenalLoadout.ApplyLoadoutString()` can apply the saved native string.
  - After a successful serialized native loadout apply, refresh the server-side `SCR_PlayerController` main entity with `SetInitialMainEntity(playerEntity)`. h-istasi applies in place, but the controller refresh still helps possession/UI systems observe the changed entity state.
  - Campaign-debug loadout apply probes should not call `loadout_save` just to create a fixture, because that writes personal fixed-slot loadout files. Seed transient `HST_SavedLoadoutState` records directly, call the real `RequestMemberApplySavedLoadout()` path, then remove those records and restore the full arsenal plus the player's issued-loadout ledger from a pre-probe snapshot. Serialized apply uses the normal transaction path, which returns every previously issued finite item not present in the new loadout; restoring only the debug test prefabs can zero the player's issued ledger and duplicate refunded arsenal.
  - To test valid apply transaction semantics without leaving a new physical kit on the debug actor, capture the actor's current native serialized loadout and use it on the transient valid loadout while arranging explicit finite-cost slots for ledger assertions. This proves command, native apply, withdrawal, issued-ledger, and cleanup behavior without requiring `loadout_save`.
  - To test physical saved-loadout reflection, seed a second transient non-serialized loadout with explicit compact finite slots, apply it through `RequestMemberApplySavedLoadout()`, count the player inventory before/after, then apply a transient restore loadout that uses the original native serialized string. Keep this debug-only and restore arsenal/issued state afterward; rendered editor UI and exact equipment-slot visual proof remain separate.
  - Non-serialized saved-loadout apply inserts every saved slot through the normal inventory insertion path. Do not try to create capacity by adding a backpack/carrier as another non-serialized slot: the backpack itself is inserted as cargo and can fail first with `Inventory Full`. Use compact finite fixtures for the physical reflection probe; if a future test needs guaranteed new storage, arrange it through an equipped/serialized loadout step or mark the physical capacity prerequisite explicitly.
  - Physical saved-loadout debug probes need a live inventory capacity preflight before seeding finite items. A character with no cargo-capable clothing/webbing/backpack should make physical apply, live draft apply, physical restore, and live draft restore assertions `BLOCKED`, not `WARN` or `FAIL`, because applying loose magazines to an entity with no storage correctly returns `Inventory Full`.
  - After the physical saved-loadout apply, the service refreshes the loadout editor session from the live player. The debug runner can count matching `m_aDraftSlots` and storage-item slot kinds before/apply/restore to prove the server-side editor model reflects the live physical inventory without opening rendered UI.
  - Save/load/reset feedback should be a short-lived in-editor toast, not a global notification. Use a generation token when scheduling delayed clears so an older timer cannot erase a newer prompt. Disabled `Load` buttons should stay visible for empty fixed slots.
  - Loadout candidate icon hints also drive native row previews. Use preview-capable hints such as `medical`, `utility`, and `equipment`; a generic value that the client does not recognize as preview-capable will render as a flat fallback icon even when the prefab can be previewed.
  - Fixed saved loadout slots should be emitted in deterministic slot order from the service payload, including empty slots. A per-slot row can then show both `Save` and `Load`; keep `Load` visible but do not bind a user id when the slot is empty.
  - Per-slot save buttons should send the fixed loadout id such as `slot_0` through the existing `loadout_save` request. The service can treat that argument as a slot target while preserving the old global save behavior for empty arguments.
  - Saved loadout rename/load must stay server-routed through the coordinator. The client can edit a non-empty slot name inline, but the mutation should send `loadout_rename` with `slot_id:name`; load should fail visibly if the serialized native loadout cannot be parsed or applied instead of reporting success.
  - If a button icon is intentionally suppressed in script, also remove the layout fallback text and let the label span the full button. Hidden dynamic widgets can still be visible for a frame during layout debug/initial population, so stale symbols such as the candidate Edit `>` can flash or reserve dead spacing.
  - Loadout editor search should use the full recovered arsenal item arrays (`m_aItem*`), not the selected storage candidate arrays (`m_aCandidate*`). The search result click should send `add_storage_item` with the selected storage node id and prefab; if no storage is selected, show a status and do not issue the request.
  - Storage search category matching should include the raw item category, the slot category label, and the visible storage browser tab label. Grouped tabs such as Weapons and Clothing otherwise will not match items whose raw categories are `weapon`, `launcher`, `headgear`, `vest`, etc.
  - Numeric or short storage searches should match normalized display/category text, not prefab GUID/path noise. Only include prefab paths in search for explicit path-like queries; otherwise strings such as `556` can match unrelated items through resource identifiers instead of user-visible names.
  - Edit-box search inputs can be handled through `ScriptedWidgetEventHandler.OnChange(Widget w, bool finished)` after assigning a stable user id and adding the handler to the `EditBoxWidget`. Remember to clear layout-owned dynamic result containers such as `StorageSearchItems` when reusing the editor root.
  - If an edit-box render path rebuilds dynamic siblings on each query change, restore focus with `WorkspaceWidget.SetFocusedWidget(input, true)` after syncing text and attaching the handler, or continuous typing can stall after the first refresh.

- Dynamic horizontal tabs should be centered by sizing the layout-owned host to the children, not by moving each tab.
  - A tab button with a fixed width and layout padding contributes both to the desired strip width.
  - If the host is wider than the sum of the tab buttons plus their padding, Enfusion lays children from the left and leaves the extra space at the end.
  - Current example: the loadout editor mode tabs use symmetric host insets so the six layout-generated tab buttons sit centered over the left pane.
  - Fixed-size preview boxes inside the candidate header need the same negative far-edge rule as other top/left same-anchor boxes. Positive `OffsetRight` / `OffsetBottom` values produced negative runtime size for the header preview anchor, which made the current equipped item preview disappear.

- `ItemPreviewWidget` does not reliably render every inventory prefab.
  - Keep a category/fallback icon available when prefab or entity preview setup fails.
  - Do not hide the fallback icon just because the entity preview lookup returned null for a non-empty prefab.
  - Empty live equipment nodes still need a visible fallback tile plus `Empty` text; suppressing the fallback makes the header look like a missing render instead of an empty slot.
  - Attachment nodes should resolve their preview icon key as `attachment`, not as slot keys such as `optic` or `muzzle`. Slot keys are useful labels, but they are not in the native-preview eligibility list and will force an equipped attachment preview down the fallback-only path.
  - Attachment slot scans should only treat an attached entity as editable if it has `InventoryItemComponent`. Prefab child props without inventory components, such as structural handguards or fixture meshes, can exist under a weapon slot but should render as empty/non-removable in the loadout editor.
  - Worn clothing Edit means cosmetic/functional attachment-slot editing, not cargo storage editing. Route the equipped clothing item into the existing attachment child-node view; keep cargo insertion/removal in the storage tab.
  - Clothing Edit needs an internal state distinct from weapon attachment editing, while mapping the visible top tab back to Clothing. This prevents Back/click behavior from leaving the player in weapon attachment mode after editing clothing child slots.
  - Clothing attachment discovery should follow broad slot validation: an `AttachmentSlotComponent` with a real attachment slot type is inspectable even if `ShouldShowInInspection()` is false. Recurse child and owned attachment storages, but skip `SCR_UniversalInventoryStorageComponent` cargo storages so inventory pouches do not become cosmetic edit slots.
  - Worn clothing cosmetic slots can be `LoadoutSlotInfo` entries on `ClothNodeStorageComponent`, not only `AttachmentSlotComponent` entries. Treat those loadout child slots as edit nodes for clothing Edit, while continuing to filter structural pouch/scabbard/carrier slots out of add/remove candidate lists.
  - Clothing child-slot candidates should mirror native loadout slot matching: compare the child `LoadoutSlotInfo.GetAreaType().Type().ToString()` with the candidate prefab entity's `BaseLoadoutClothComponent.GetAreaType().Type().ToString()`, then still run the live `CanStoreItem`/`CanReplaceItem` check. Broad arsenal categories are too loose for these slots and can admit unrelated maps, full clothing, or utility items.
  - Spawned clothing/webbing prefabs can carry default stored items inside structural equipment slots. Clean default bayonet contents from those structural slots before insertion or preview, but do not delete the scabbard/carrier shell because it is part of the worn item appearance.
  - When editing clothing attachment children, keep the equipped parent row non-clickable. Clicking the parent while already in its child-node view should not exit into a replacement candidate list.

- Preview camera movement should be intentional.
- Auto-frame the loadout editor preview once when opening or rebuilding the preview world.
- The visible color behind the mannequin comes from the preview stage `MatSkyBox`/`SkyPreset`, not just the `RenderTargetWidget` tint or the decorative sky sphere. Render-background color settings should update the spawned `GenericWorldEntity.GetSkyMaterial()` colors or rebuild the stage with an alternate sky preset.
- Storage selection and storage item add/remove should not invalidate the preview render key or move the camera unless the visible equipped entity actually changes appearance.
- Do not include selected storage-item ids in the preview render key; doing so forces unnecessary clone rebuilds and camera motion while adding repeated inventory contents.
- Non-storage item/node selection should reset the preview camera to the default auto-frame path and use a closer entity framing distance, so choosing a weapon/attachment/clothing edit context does not leave the camera at the previous full-character view. Keep this separate from storage add/remove operations to avoid motion while filling inventories.

- Native input hints should use the widget-library input button components when possible.
  - `SCR_InputButtonComponent` can bind an action name and label to the current input device.
  - Pass configured input action names to `SetAction`, not literal keys. The component resolves the current keyboard, mouse, or controller glyph.
  - Keep generated hint widgets passive with `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` when they are only visual footer/context hints.
  - Hand-drawn key boxes are useful as fallbacks, but they do not update like native button hints.
  - If a persistent footer or row container is repopulated, clear its child widgets before adding native hint widgets; stale children remain visible even when the backing arrays were cleared.

- Avoid clearing action keys during UI teardown unless that UI will continue owning input for the next frame.
  - `Debug.ClearKey(KeyCode.KC_I)` is acceptable while setup is actively blocking command menu input, but clearing it after setup finalization can consume the first legitimate command-menu open press.
  - Prefer resetting the destination UI input latch and rebinding its input context after a modal/setup screen closes.
  - If a raw key bridge is needed for the destination UI, clear the raw key only after the destination UI actually handles a toggle. A post-setup recovery method can run on the same frame as the user's first real press and eat that press if it calls `Debug.ClearKey` speculatively.

- `ItemPreviewWidget` setup is not proof that a prefab will render useful pixels.
  - Put a category/fallback image behind the native preview widget and keep it visible at low opacity after calling `SetPreviewItemFromPrefab` or `SetPreviewItem`.
  - This preserves a visible row affordance for unsupported prefabs without adding script geometry or hiding native previews that do render.
  - Do not hard-exclude worn equipment categories from native previews. Clothing, storage containers, and weapons should attempt native preview first with the fallback underlay still present, because a visible character item may have a valid preview prefab even if some inventory entries still fall back.
  - For equipped items and live inventory contents, prefer entity previews before prefab previews. Clear the preview widget, then call `SetPreviewItem(previewWidget, entity)`.
  - For arsenal candidates that only have prefab resources, call `SetPreviewItemFromPrefab(previewWidget, prefabResource)` and leave the fallback underlay visible.

- Slot edit remove actions must be independent from compatible replacement candidates.
  - A slot can have a removable equipped item even when the arsenal has no valid replacement entries.
  - Gate the UI remove control on the selected item existing, then let the server-side inventory path reject only truly invalid removals.
  - Header text for an edit context should show the slot/category label and equipped item display name separately; otherwise an equipped item can look like an anonymous preview tile.

- Reused mode panels should hide stale child content on mode reset and re-show the parent before populating.
  - A child such as settings content can remain marked visible while its parent panel is hidden, which makes logs look populated but the screen appear blank.
  - Reset both the parent region and mutually exclusive child panels before rendering the active mode.

- Keep code comments sparse and practical.
  - Comments should capture non-obvious engine constraints, not restate simple assignments.

## Campaign Debug Runners

- Long-running campaign smoke/debug actions should run as a server-side sequencer, not as one huge UI command RPC.
  - The command menu button should start the run or return current status; the coordinator tick should advance one bounded step per second.
  - Keep runner state runtime-only unless the test result itself needs persistence. Do not add fields to save data for a transient debug harness.
  - Current example: `HST_CampaignCoordinatorComponent.RequestAdminRunCampaignDebug()` starts the sequenced campaign debug runner, and `TickCampaignDebugRunner()` advances bootstrap, reports, HQ/spawn, economy/support, phase 0-13 mechanics, mission sweep, phase smoke, and final report stages.
  - Global fallback policy: fallback paths are resilience code, not proof of success. If a runtime path uses a fallback, the primary method failed and campaign debug should record that as degraded evidence, `WARN`, `BLOCKED`, or `FAIL` depending on whether the primary behavior is certification-required. Do not let fallback-produced state silently satisfy primary-method assertions.
  - Profile selection should be explicit in both runtime status and artifacts. Current campaign debug profiles include `smoke`, `admin_smoke`, `foundation`, `faction`, `faction_physical`, `physical`, `support_physical`, `mission_matrix_state`, `mission_matrix_physical`, `civilian_undercover`, `arsenal_garage_build`, `persistence_inprocess`, `full`, `full_certification`, `post_restart_verify`, `persistence_restart_external`, `background_soak`, and `external_required`. External/restart/soak profiles must report `BLOCKED`/external-required evidence until a real launcher, server restart, client reconnect, or long-window soak proves them.
  - The visible admin menu should expose one obvious full-run action, currently `Run Full Campaign Debug` backed by `full_certification`. Keep narrower actions labeled as scoped debug profiles so playtesters do not mistake them for equivalent full-suite buttons.
  - The commander mission menu should expose mission categories, not every target zone. A category command should select a mission definition in that category, then choose a valid eligible target inside the configured mission-selection radius. Keep the old direct-zone mission commands available only for debug/back-compat paths unless a UI explicitly needs them.
  - Runtime settings schema 14 owns `world.playerRenderBubbleRadiusMeters` and `world.missionSelectionRadiusMeters`. If the mission-selection radius is unset or non-positive, normalize it to the player bubble radius so category missions default to nearby rendered gameplay.
  - Runtime settings schema 15 owns default campaign outcome mode: `populationOutcomeEnabled`, `victoryPopulationSupportPercent`, and `legacyControlVictoryEnabled`.
  - Runtime settings schema 16 owns `features.trackResistanceSupportGroupsOnMap`, defaulting to enabled for generated and migrated configs.
  - Runtime settings schema 17 removes `campaign.defaultHideoutId`. Initial HQ placement is selected through the setup map and persisted in campaign state, so generated settings should not load, write, or display a default hideout id.
  - Runtime settings schema 18 adds JSON-safe `_comment` and `_comment_*` fields to generated settings. Do not use raw `//` or block comments in profile JSON; keep comments as ignored string fields so the line-scanning loader and external JSON tools remain compatible.
  - Location taxonomy is a gameplay contract, not just map decoration. Towns must remain town/support zones, resources and factories must not be promoted from settlement names, and radar/ferry/dynamic-site pools should be mission-site/bookkeeping targets without start garrisons. Extra zones are acceptable, but curated minimum counts and known IDs should be asserted by Full Campaign Debug preflight.
  - Static location marker icons should not reuse QRF/attacker tactical icons. Keep QRF-style response markers on `OBJECTIVE_MARKER`, but publish towns as `POINT_OF_INTEREST`, base/outpost/seaport/airfield locations as `OBSERVATION_POST`, and mission-site bookkeeping anchors as `POINT_SPECIAL`; Phase 23 should assert no static town/base/mission-site zone marker still uses the QRF icon.
  - Default campaign-end evaluation is population-first. Victory requires the FIA-supporting share of remaining town population to meet the configured threshold and all airfields to be resistance-owned. Loss requires killed population greater than one third of initial town population.
  - Keep legacy control-percent victory behind the explicit legacy toggle.
  - Full Campaign Debug Phase 24 should arrange neutral, victory, and catastrophe population fixtures before calling the real strategic outcome evaluator, and assert save-data roundtrip preservation of terminal population/airfield metadata.
  - Missing native Game Master budget editor diagnostics are not gameplay proof. Record them as `preflight.gm_budget.editor_diagnostics` with `BLOCKED`/diagnostic evidence and exclude that row from certification; the live disabled-budget gameplay contract is `preflight.gm_budget.policy` plus `preflight.gm_budget.disabled_spawn_probe`.
  - Long runs should expose status, cancel, and cleanup admin commands beside the start action. Cancel should stop the sequencer and write artifacts; cleanup should complete the current/early debug missions and clear player-requested support requests.
  - Assign each campaign debug run a deterministic `hst_debug_` marker/mission prefix and record it in status plus artifacts. Retag forced debug-started missions before objective/runtime initialization so derived objective, runtime entity, asset, active group, and marker IDs inherit the prefix and can be cleaned later.
  - Retag debug-created support requests and enemy orders before marker refresh or report generation. Support physicalization derives group IDs from `m_sRequestId`, so prefixing the request before physicalization lets later group and marker cleanup stay deterministic without changing normal gameplay IDs.
  - Debug seed commands that intentionally queue enemy orders or enemy support should isolate the selected target's support ledger first. Reset only the debug target ledger's stack cooldown/spent counters while the runner is active; normal enemy support cooldown and max-spend behavior stays covered by the dedicated support-spend contract.
  - Prefix cleanup should be narrow and evidence-producing: remove only persisted records whose IDs contain the debug prefix, report before/after/removal counts as typed cleanup metrics, and rebuild campaign markers after marker-backed records change. Do not claim this is a generic world scan for arbitrary untracked physical entities.
  - Normal non-restart campaign debug profiles must also clean `hst_smoke` persistence sentinels before final certification. `persistence_restart_external` is the only profile that intentionally preserves those sentinels, and it must report that state as external/restart-pending rather than as a cleanup PASS.
  - Defend Petros has static marker IDs (`hst_defend_petros`, `hst_defend_petros_attackers`) linked to dynamic mission/group IDs. Prefix cleanup must clear `m_sDefendPetrosMissionId`, order/support/group links, and the active flag before rebuilding markers; otherwise marker rebuild can recreate static markers that still point at removed debug-prefixed records.

- Campaign debug certification output should be structured first, text second.
  - Workbench can fail with a native crash during Game script validation when large debug-verification probe batches are added, even if repository-side text/brace checks pass. Reintroduce certification slices in small increments and confirm Workbench completes Game script compilation before stacking more probes.
  - A single Workbench log directory can contain several script reload attempts. When auditing a compile failure, split by the latest `Reloading game scripts` / `Script validation` segment before deciding whether an earlier `SCRIPT (E)` line is still current. Record which later reload proves the fix, and keep later commits unproven until they have their own reload/runtime evidence.
  - If Workbench crashes after Game script compilation with no `SCRIPT (E)` rows, a compile-valid h-istasi change can still be the trigger. First halve or back out the most recent script slice and retest the same loaded-project set; profile project-list isolation is a secondary check, not proof that the mod is innocent.
  - Protected helper names are class-local. If a campaign-debug report path calls a helper such as `ReportBool` or `ResolveEntityPrefabName`, the calling class must define it directly; a same-named helper on another service does not satisfy the caller and Workbench reports `Undefined function`.
  - Workbench Game script compilation can report `Broken expression (missing ';'?)` on a valid-looking helper declaration when a newly introduced parameter spelling trips the parser at a method boundary. Current observed case: `EnsureCampaignDebugArtifactRecorded(string artifactPath)` failed at the declaration; the previously compiled `string path` form is the safe spelling for that helper, and the validator guards it.
  - Keep transient result models outside save data and serialize them with `JsonSaveContext` under `$profile:h-istasi/debug`.
  - Current artifact contract: `HST_CampaignDebug_<runId>.json`, `HST_CampaignDebug_<runId>_summary.txt`, and `HST_CampaignDebug_<runId>_state_diff.txt`.
  - The typed result layer should record run/case/assertion/metric fields, while legacy command/report strings can be wrapped as typed cases during migration.
  - The text summary should include `critical failures <n>` and a `failure details` section for FAIL/BLOCKED cases with feature, stage, expected, actual, reference IDs, position evidence, likely reason, and suggested next inspection command.
  - Summary artifacts should derive their feature, mission, physical AI, cleanup, and failure-inspection matrices from typed case results so gaps stay visible instead of being buried in freeform report text.
  - Manual external gaps, such as real process restart, second-client reconnect, and long endurance soak, should be explicit typed BLOCKED cases. Do not wrap them as successful legacy text rows, because the summary needs to show those gaps as incomplete external coverage.
  - Mission sweep runtime/primitive proof must not treat "already completed before sampling" as a certification warning. A primitive that disappears or completes before runtime action proof is a FAIL unless it is explicitly `abstract_fallback`; add a debug hold instead of downgrading the evidence.
  - State-diff text is enough for forensic triage: capture start and end money, HR, training, war level, active mission, asset, group, support, order, and marker counts.
  - Baseline preflight must include runtime-selected resources, not only definition/catalog resources. The campaign debug preflight should hard-fail if mission runtime prop/vehicle prefabs or AI waypoint prefabs used by primitive services fail `Resource.Load`.
  - Baseline persistence should be a typed case around `HST_PersistenceService.BuildPersistenceReport()`: assert server/member/service readiness, non-empty report text, no failure strings, and explicit native-vs-profile-fallback mode. Native unavailable with profile fallback available is a WARN, not a PASS or a generic failure.
  - Non-legacy typed cases should emit a `post_case_cleanup.*` leak probe while the campaign debug runner is active. The probe should allow the mission intentionally under test and pre-existing active missions captured at run start, then check for unexpected active missions, orphan mission assets, orphan active groups, orphan linked markers, and backing records missing markers.
  - When a phase smoke step has a typed case builder, keep its command/report string as recent-log or case evidence only. Do not also emit a duplicate legacy pass/fail case for the same action; duplicate rows inflate the summary matrix and bypass the typed cleanup model. Current phase smoke uses typed cases for valid indices 0-62, including persistence seed/run/report steps and Phase 20/21 report steps.
  - Debug-only physical entities need a queryable runtime name for final cleanup. `IEntity.SetName()` is available at runtime; set names with `HST_CAMPAIGN_DEBUG` plus the active `hst_debug_` id before relying on the cleanup world scan. The scan must delete only entities whose names contain the explicit debug tag or cleanup prefix, not by broad prefab class.
  - Do not treat a legacy string-wrapped report case as full certification for a feature. Full coverage needs a direct ARRANGE/ACT/OBSERVE/ASSERT/CLEANUP case with state and physical-world evidence, or the feature should remain WARN/BLOCKED/not-covered in the verification audit.
  - Raw PASS/WARN/FAIL totals are not a certification verdict by themselves. Each assertion should carry a proof level, observed path, required path, and whether it counts toward certification. WARN and external-process gaps can be useful diagnostics, but they must not inflate the proven certification numerator.
  - Debug artifacts with empty build provenance fields predate the build-info proof path. Treat them as stale evidence even when the case matrix is useful for diagnosis; the server log and JSON artifact must include non-empty build SHA/UTC/label before a runtime run can close a static-validated fix.

- Physical runtime probes should not silently pass when there is no controlled player entity.
  - Bootstrap should mark a physical-blocked flag if the controlled player cannot be resolved after teleport/spawn setup.
  - `SCR_Global.TeleportPlayer()` can return true before the server-side controlled entity origin reflects the new position in the same script slice. If a debug probe immediately performs distance checks or interactions after teleporting, verify `ResolveControlledPlayerEntity().GetOrigin()` against the resolved ground position and force `SetOrigin()` for the probe when needed.
  - On a dedicated server, a server-side `SCR_Global.TeleportPlayer()`/`SetOrigin()` confirmation is not enough to prove the owning client view moved. Queue an owner RPC on the player-owned request bridge and log the client-side result. The expected proof is a server campaign-debug teleport line with `owner RPC 1` plus a client `h-istasi campaign debug teleport owner ... confirmed 1` line.
  - Continue non-physical state/report checks, but mark convoy, captive, and other physical probes as `BLOCKED` instead of converting missing player context into a pass.
  - HQ runtime checks should read tracked entity handles from `HST_HQService`, not just campaign-state positions. A rebuild case should assert tracked Petros/cache/arsenal/tent/spawn-point runtime keys, their actual positions against expected HQ offsets, per-slot nearby world duplicate counts, and arsenal readiness/action-surface status.
  - Keep existing-HQ runtime proof separate from HQ rebuild placement proof. If the rebuild command is blocked by dry-ground/build placement validation while the existing Petros/cache/arsenal/tent/spawn-point runtime objects are tracked and unique in the world, classify the rebuild case as `BLOCKED` and leave the hard runtime-object flag assertions to `hq.runtime_objects_existing`.
  - The player spawn service submits `SCR_FreeSpawnData` for actual player possession, but the HQ runtime service should still keep a physical FIA spawn-point entity near HQ so the command menu and campaign debug suite can prove the respawn surface exists after HQ rebuilds.
  - Explicit HQ runtime clears must reset Petros' missing/last-spawn debounce state. A manual rebuild can otherwise delete a valid Petros handle and then refuse to replace him while the debug runner advances in short same-second slices.
  - Petros should be spawned through a dedicated one-slot `SCR_AIGroup` prefab and then resolved from the group's controlled agent. Spawning a standalone character and attaching it afterward can produce a valid-looking handle that is removed by the next HQ lifecycle tick.
  - While the runner is active, HQ-stage command-menu checks should build the real admin-tab visible payload and assert campaign debug start/status/cancel/cleanup controls are still present, then cross-check command coverage for missing visible/dispatch entries.
  - Ground support physicalization only creates and links an active-group record. A campaign-debug support probe must run `HST_PhysicalWarService.UpdateRoutedActiveGroupsNow()` before asserting physical population, then resolve any `spawn_pending_agents` group through native finalization, one native `SpawnUnits()` retry, and stock member-slot population before route movement is judged. Do not create direct faction-infantry fallback inside the certification probe; if primary paths have not produced agents yet, report the group as pending instead of certifying route movement.

- Full-campaign debug coverage should explicitly map to the phase plan instead of assuming late smoke helpers cover everything.
  - Phase 0-13 coverage needs its own sweep for foundation/checkpoint state, mission runtime visibility, convoy route/readiness/waypoint/contact/completion behavior, active-mission persistence, non-convoy primitive runtime, zone activation, garrison recruit/remove, civilian aid, support cancellation, vehicle/loadout actions, and command UI coverage.
  - Phase 0 foundation/checkpoint should remain a typed case, not only a report string: assert active schema/HQ/Petros state, checkpoint acceptance, persistence status evidence, last-save timestamp refresh, captured save snapshot schema/HQ/Petros state, and active mission/group count stability.
  - Phase 1 mission-runtime visibility should remain a typed case, not only concatenated reports: assert mission/UI/runtime/objective service readiness, definition count, report prefixes, explicit zero-active reporting, active mission identity/runtime metadata, fallback/failure hard failures for certification-required runtime paths, and orphan objective/asset/runtime-entity counts.
  - Early-mechanics steps that already emit typed cases should not also emit generic action/observation classifier cases over the same returned report text. The typed case is the authoritative PASS/WARN/FAIL source; duplicate wrappers can create extra failures from embedded detail text without adding new evidence.
  - Phase 13 primitive sampling should emit a wrapper-level typed case in addition to the primitive-specific physical probe: assert sample mission start, debug-prefix tagging, selected runtime report evidence, primitive metadata, objective/asset/runtime-entity counts, and final succeeded status. Keep captive follow/extraction behavior in the dedicated rescue/captive case.
  - Captive follow probes should use repeated bounded runtime samples after player displacement, not a single tick. Record sample history, best distance, max movement, max distance closed, and fail hard if the captive neither closes distance nor remains near the player before timeout.
  - Captive vehicle boarding can return accepted through the server-authoritative compartment path before `SCR_CompartmentAccessComponent` exposes seated/getting-in state in the same script slice. The debug probe should still assert the asset's authoritative loaded carrier association and use that as synchronous proof when immediate compartment state is not observable.
  - Captive follow is driven by a callqueue-backed entity component, so mission-runtime ticks that repeatedly call `StartFollowing()` must pulse the existing same-target follow controller instead of returning early. If repeated follow ticks show no owner movement while the captive is still far from the target, apply a bounded direct catch-up step through `HST_WorldPositionService.ResolveSafeGroundPosition()` and log `direct catch-up after stalled follow`; this is a gameplay recovery fallback and should be visible as runtime evidence, not hidden as natural pathfinding.
  - Mission-sweep start/runtime rows should be typed cases, not legacy result rows. Assert definition metadata, start command acceptance, active mission record, target zone/position, linked marker evidence, objective/asset/runtime-entity counts, runtime primitive/type metadata, clean spawned runtime state, and primitive-specific prerequisites before running the primitive or convoy physical probe.
  - Mission-sweep primitive probes should call the real runtime action path for each primitive, not admin-complete the mission and infer success. Current examples: `kill_hvt` uses `RequestServerMissionAssetDestroyed`, `destroy_target` uses `RequestServerMissionAssetExplosiveDamage`, `rescue_extract` uses the captive interaction probe, and transport primitives use `mission_asset_load` / `mission_asset_deliver` when a nearby carrier can be arranged.
  - Transport primitive debug carriers can be physically moved faster than the native sphere query updates. After a successful physical carrier scan has registered a `mission_carrier` runtime record, the mission runtime load path may use that nearby runtime carrier record as a narrow fallback when the immediate entity query misses it. Do not broaden this to arbitrary runtime vehicles.
  - Mission-sweep runtime and primitive probes that observe `SUCCEEDED` before typed primitive sampling should FAIL unless the primitive is explicitly `abstract_fallback`. Real runtime completion is still useful evidence, but the certification runner did not observe the required primitive action path; add a debug hold/release point instead of downgrading the missed sample to WARN.
  - The coordinator may hold auto-completion only for the current campaign-debug mission instance (`m_sCampaignDebugCurrentMissionInstanceId`/`m_sCampaignDebugEarlyMissionInstanceId`) before primitive proof. The runtime case should emit `mission.runtime.debug_hold` when completed objectives are intentionally kept active for sampling; normal missions must continue completing through the regular runtime path.
  - Every runtime mission completion candidate must pass through the same campaign-debug hold gate, including player mission interactions, server asset-destroyed events, explosive damage events, primitive refresh ticks, and area objective ticks. Release the hold only after the active primitive snapshot is recorded, then let the real primitive action path complete the mission and prove rewards/status.
  - Runtime primitive semantics should win over stale or generic objective enum values when mission assets make the outcome unambiguous. Restored or dynamic missions can carry a generic objective type while their primitive and asset role still say `destroy_target`/`destroy_target` or `recover_cargo`/`logistics_cargo`; pollers and counters must count destroyed or delivered assets in those cases.
  - Objective-to-asset role resolution should check explicit target IDs and runtime primitive first, then fall back to the objective enum. Otherwise a generated `CLEAR_AREA` objective for `destroy_target` or `recover_cargo` will look for a hold marker and never complete after the real asset interaction succeeds.
  - Logistics mission completion already pays the configured definition reward shown in mission text. Do not add a hidden hard-coded money/HR bonus from the generic completion outcome unless the reward text and debug assertions are updated to include it.
  - Destroy-target primitive probes must drive every still-unsatisfied role-matching `destroy_target` asset, not only the first asset returned by the mission sweep helper. If the selected asset was already destroyed, tick the normal mission runtime/objective path so asset-backed objective completion can catch up before reward/status assertions.
- Mission cleanup cases must classify the completion path. `already succeeded` means runtime/probe completion happened before cleanup, `completed 1` from the cleanup command is WARN-only admin cleanup evidence, and `completed 0` is a cleanup failure. Do not use an admin cleanup success row as proof that the physical mission behavior passed.
- For debug-prefixed mission instances, mission cleanup should remove mission-owned active-group records and their runtime group entities before asserting `mission.cleanup.groups`. Counting live debug mission groups without deleting them leaves avoidable post-case leak noise.
  - Transport primitive certification depends on a physical carrier near the player. Arrange a temporary vehicle carrier near the pickup point, run the real `mission_asset_load` path, move the carrier/runtime-vehicle record to the delivery point, run `mission_asset_deliver`, then delete the temporary carrier and remove its runtime record. If the carrier cannot be arranged, record the case as `BLOCKED` with the load-command evidence instead of claiming a cargo/reward failure or mutating the asset directly. Natural player driving/path travel remains a separate physical-behavior gap until sampled.
  - Transport primitive probes must deliver every role-matching cargo asset required by the objective, not only the first asset returned by the mission sweep helper. Recover-cargo and supply missions can initialize multiple assets for a single objective count, so completion/reward assertions should run only after the probe has repeatedly moved the same temporary carrier through load/deliver for all undelivered role assets.
  - Mission asset loading uses `HST_MissionRuntimeService.VEHICLE_CARRIER_RADIUS_METERS` (10m), not the looser player asset interaction radius. Debug-created temporary carriers must be placed against the verified current player entity position and remain inside that carrier radius after terrain/vehicle-spawn resolution.
  - `hold_area` and `clear_area` primitive probes should sample a living controlled player inside the objective radius, mission-owned hostile presence/absence, objective hold seconds/world detection, runtime tick completion, rewards, and cleanup. If the probe uses controlled hostile neutralization or has no hostile population to observe, keep area objective logic separate and report `primitive.area.physical_combat_observed` as BLOCKED until natural combat/contact proof exists.
  - Phase 12 persistence smoke should seed sentinel state first, then record typed assertions against the actual `HST_CampaignState`: expected-summary task, active smoke missions, convoy and primitive mission matrices, mission assets/runtime entities, garage cargo, support/order sentinels, civilian records, and undercover records. Use `HST_CampaignSaveData.Capture()` plus `Restore()` for an in-memory roundtrip and compare the smoke summary/report/counts between live and restored state. The immediate and restored smoke/report commands may return `WARN` if live systems mutate counts between seed and report, but that is drift evidence only when `missing/zero none` remains true; the live-vs-restored summary/count assertions are the hard persistence-fidelity proof.
  - Later phase-persistence smoke runs can legitimately drift from the original expected summary as other phase probes spawn groups, resolve missions, change garrisons, or advance capture progress. Treat a persistence smoke `WARN` report with `missing/zero none` as warning evidence for expected/current drift, not as a failed persistence case. Missing data, duplicate smoke records, or an explicit `FAIL` report remain hard failures.
  - Generated-content debug coverage should assert `HST_CampaignState.m_aGeneratedSites` and `m_aGeneratedRoutes` directly: per-zone primary/roadblock/support/secondary anchors, unique IDs, valid non-zero site positions, source metadata, route links, vehicle-safe routes, at least three waypoints, coherent waypoint indexes, and resolved zone links. Treat `BuildContentReport()` as evidence only.
  - Generated site offsets can land in water or unsafe terrain near coastal zones. Resolve primary/source site anchors through dry safe-ground search around the preferred offset and then the source zone, with a road-safe fallback before accepting the final ground fallback. The road-safe fallback can trust `TryResolveNearestRoadVehiclePosition()` because it already rejects water, narrow roads, and unstable vehicle footprints; repeating `IsDryGroundPosition()` after a successful road resolve can falsely invalidate usable road-side anchors. Resolve generated route waypoints through `TryResolveNearestRoadVehiclePosition()` before falling back to large-vehicle-safe terrain.
  - Generated road-route validation must honor the same road-footprint contract used during route creation. A waypoint snapped by `TryResolveNearestRoadVehiclePosition()` can be valid for routed driving even if the broader `TryResolveLargeVehicleSpawnPosition()` pad check or a separate dry-ground probe fails there; the road resolver already rejects water, narrow roads, and unstable vehicle footprints. Validate road-route waypoints by rechecking nearby road vehicle safety before falling back to large-spawn validation.
  - Generated route repair should not give up after checking only the original bad waypoint. Coastal/offshore offsets can be too far from usable terrain, so retry from the adjacent route destination, the generated route endpoints, and bounded radial candidates around those anchors. Standard `TryResolveVehicleSpawnPosition()` is acceptable as a route-waypoint fallback after road and large-vehicle checks because route waypoints certify passable ground, not a heavy vehicle parking pad.
  - If route generation uses close/medium/far road searches, validation must use an equivalent search radius instead of each mission waypoint's small interaction radius. Otherwise valid generated road waypoints can be reclassified as invalid only because the second pass searched too narrowly.
  - `EnsureGeneratedContent()` must not treat persisted/generated sites and routes as immutable forever. Revalidate saved generated content during foundation setup and regenerate it when per-zone anchors are missing, site validity flags drift from current rules, route links break, or vehicle-route validation fails; otherwise old save state can keep failing new route/site safety checks.
  - Area primitive debug probes should remove the blockers that `PollHoldAreaObjective()` and `PollClearAreaObjective()` actually check, then still run the normal mission runtime/objective tick. That means neutralizing debug mission groups plus nearby hostile active groups, target-zone hostile active groups, and hostile target-zone active/garrison projection before asserting the timed area-control path. Otherwise a player can be inside the radius and hostiles can look cleared for the mission-owned group while the runtime correctly holds progress at contact.
  - Phase 17 capture smoke must not select a zone with an active incomplete `conquest_` mission. `HST_ZoneCaptureService` intentionally caps resistance progress at 90/100 until the conquest objective is complete, so direct force-progress certification should pick a non-gated enemy capturable zone and assert that gate condition before expecting ownership flip and starter FIA garrison creation.
  - The early zone-activation step should exercise the real render-bubble path, not only `SetZoneActive()`: select a non-mission inactive zone with abstract garrison outside the HQ/player bubble, run `HST_PhysicalWarService.UpdateZoneActivation()` with the player far, near, then far again, report active/spawned/pending group counts, assert inactive -> active/spawned-or-pending groups -> inactive cleanup, and restore the selected zone/garrison state. Do not force pending group population from the coordinator render-bubble probe until that slice is Workbench-proven; use the dedicated physical-war population probes for forced pending-population resolution. Mission-runtime expired player-bound asset policy should be probed inside `HST_MissionRuntimeService`, because its helpers are protected there: create a debug-prefixed expired rescue mission/asset, assert near-player continuation and interaction eligibility, assert outside-bubble stop/no eligibility, assert player-carrier continuation, then remove temporary records. Convoy-expired contact cleanup policy belongs in `HST_PhysicalWarService`: create temporary debug-prefixed expired convoy contact groups with temporary runtime crew entities, exercise the same preserve/delete branch for only those groups, assert inside-bubble preservation and outside-bubble deletion, then remove temporary records.
  - Render-bubble activation may create active groups whose agent population is still `spawn_pending_agents` on the same script slice. Count pending groups separately and report near-activation as WARN/pending when the zone is active and groups exist but no spawned agents or active-force counts are visible yet.
  - Directly spawned `AIGroup` prefabs can report zero agents on the first one-second callqueue callback, especially while the resource-generation queue is busy. Keep active groups in `spawn_pending_agents` and retry agent counting for a bounded grace window before deleting/folding the runtime entity as a real zero-agent failure.
  - When an active group still fails after the pending-population window, the log must preserve the last native `SpawnUnits()` or direct faction-infantry fallback reason. A generic `zero agents after grace` line is not enough proof; it must include whether the fallback was queued, skipped because the runtime group handle was missing/non-`SCR_AIGroup`, blocked by initialization policy, or failed to spawn replacement infantry.
  - For render-bubble cleanup, sample a short repeated leave window after moving the player outside activation radius. Record sample count, window seconds, max remaining group counts, last observed state, and fail `render_bubble.zone_leave.cleanup_timeout` if active groups or active force counts remain after the full window.
  - Later phase smoke helpers can then focus on the dedicated Phase 14-24 systems, while the final report represents the Phase 25 full-campaign soak summary.
  - Phase 24 typed coverage should assert early/mid/late seeded resource/control profiles, controlled low/mid/high escalation ticks, aggression decay, and forced victory/loss end metadata. Treat long-window autonomous campaign pressure and broad physical follow-on behavior as separate WARN/not-covered gaps until they are sampled over many normal campaign loops.
  - Phase 24 escalation should keep resource-income scaling strict but should not require individual order counts to be monotonic across low/mid/high profiles. Commander ticks spend resources on orders with different costs and duplicate guards; the hard assertion is that each profile creates real orders, while attack/support income deltas prove pressure scaling.
  - Verification audits should distinguish implemented harness coverage from proven full-contract completion. Static code evidence can close wiring and typed assertion gaps, but physical/runtime slices should only be counted as fully closed after a Workbench script reload/runtime run or a generated debug report proves the case passed. Keep natural-behavior, rendered-UI, restart, multiclient, and long-soak requirements as WARN/open until direct evidence exists.
- Dedicated server/client logs are the source of truth for production parity. A Workbench-only green result is not enough for physical AI spawning, player connect/spawn races, custom input routing, marker publication, or setup/active campaign lifecycle behavior; the audit should call out whether evidence came from Workbench or from a dedicated server/client run.
- `HST_CAMPAIGN_SETUP` is a real non-combat phase. The coordinator may tick persistence, setup player-spawn sweeps, setup UI/markers, and campaign-debug control flow, but normal mission/economy/physical-war/civilian/runtime services must not tick until setup confirms HQ deployment and the state reaches `HST_CAMPAIGN_ACTIVE`.
- Physical-war entry points that can spawn or advance runtime groups should defend this contract themselves. `UpdateZoneActivation()`, mission target activation, and mission convoy ticking should return without side effects unless the campaign phase is active, so a missed coordinator guard cannot spawn US/USSR/FIA active groups during HQ selection on a dedicated server.
- Player-controller components cannot rely only on `SCR_PlayerController.GetLocalPlayerId()` during connect/setup transitions. The local id can be late or stale even while `GetGame().GetPlayerController()` and a local controlled entity exist. For client-owned HST controller components, resolve local ownership by checking the native local player controller, the component attached to that controller, the native local player id, and finally `PlayerManager.GetPlayerIdFromControlledEntity(SCR_PlayerController.GetLocalControlledEntity())`.

- Debug runners should accelerate long convoy waits through runtime state, then let normal services process the result.
  - Real convoy staging idle can be several minutes; a one-button test should advance the sample convoy mission counters to the departure threshold and wait a tick or two for mission/physical-war services to move it into the next phase.
  - This exercises the real route/readiness/contact reporting path without making admin diagnostics wait for natural convoy timers.
  - Keep physical convoy internals behind a narrow debug probe API on `HST_PhysicalWarService` instead of widening every runtime helper. The probe can use readiness/progress internals to assert vehicle assets, runtime vehicle entities, crew groups, alive crew, seated drivers, routes, waypoints, and progress samples.
  - Convoy movement proof should include readiness and progress, not just spawned object counts. Treat missing progress samples as WARN while the async runtime catches up, but hard-stuck progress should fail the certification case.
  - For repeated convoy movement proof, let the campaign debug runner wait across real five-second convoy progress intervals after forcing departure. `HST_ConvoyProgressStatus` can then store sample count, max movement, max distance closed, and sampled phase history for the typed probe. Do not synthesize movement in the probe itself; the assertion should report WARN if the engine did not physically move far enough during the window.
  - The controlled one-button convoy movement window should run longer than `CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS` before teleporting the player for contact. Otherwise a no-movement convoy is sampled only inside pre-recovery grace and the report cannot prove whether route reissue would recover it.
  - Convoy phase-chain assertions should distinguish evidence levels: travel evidence can come from sampled `convoy_moving`/`convoy_contact`/terminal phases or the final mission phase, contact-or-terminal evidence should remain WARN until contact/arrival/elimination is observed, and terminal evidence should stay WARN unless `convoy_arrived` or `convoy_eliminated` is actually sampled or final.
  - A separate controlled convoy phase-chain probe can prove the state-machine helper contract by creating temporary debug convoy records, calling `UpdateMissionConvoyPhase`, `SetMissionConvoyMoving`, `SetMissionConvoyContact`, and `UpdateMissionConvoyObjective`, then cleaning those records. Keep this evidence labeled as controlled state-machine coverage; it must not replace natural physical-driving movement samples or the live arrival/failure path.
  - A fully sampled convoy movement window with no vehicle movement, no destination-distance closure, no recovery attempt, and no contact/arrival/elimination phase progress is a real `convoy.movement.stall_timeout` FAIL only after the sampled window reaches the convoy recovery/reissue threshold. Short repeated samples before that threshold should remain WARN/pending with evidence, because recovery cannot have fired yet.

- POW/captive debug certification should use the real interaction commands.
  - For `rescue_extract`, call `mission_captive_extract` first; that path frees an unpicked captive and marks the asset picked up.
  - Then call `mission_captive_follow`; the expected state is `m_sLastInteraction == "following"`, `m_bAttachedToCarrier == true`, and a non-empty carried-by identity.
  - Checking only mission text or spawned captive asset counts does not prove the rescue primitive can move from freed to following/extracting.
  - A full rescue probe should iterate every required captive asset, use the same free/follow interaction path, teleport the debug player to the asset delivery target, and call `mission_captive_extract` again for each attached captive. Assert delivered counts, `m_iExtractedCaptiveCount`, mission success, alive/not-destroyed captives, and exact money/HR reward deltas.
  - A synchronous debug helper can tick `HST_MissionRuntimeService` after moving the player to prove the follow link survives and the follow controller/waypoint path is processed. Do not hard-pass physical distance closure unless the measured captive position actually moves closer; AI walking may need real frame time beyond a single script tick.
  - If the captive remains attached/following and inside the follow break range but does not physically close distance during the synchronous sample window, record WARN timing evidence instead of FAIL. A broken follow link or out-of-range captive remains a real failure.
  - To certify captive vehicle boarding without mutating the real rescue mission, create a debug-prefixed temporary captive asset/runtime entity and a temporary carrier, call the same `TryMoveCaptiveIntoVehicle` helper used by normal follow updates, then assert `SCR_CompartmentAccessComponent` seat/getting-in state and loaded carrier state. Dedicated/server-side probes should try `GetInVehicle(slotOwner, slot, true, ...)` before the animated `MoveInVehicle()` path, run a bounded compartment-state rescan, and fail the boarding/transport case if no seated/getting-in state is visible after that rescan. Cleanup should call `GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false)` when seated, delete the runtime entity/carrier, and remove temporary mission asset/runtime records before recording the case result.

- Player-requested support cooldowns can poison later support smoke steps.
  - Stage 3 economy income certification should use `HST_TownService.CalculateResistanceIncome()` and `CalculateResistanceHRIncome()` after arranging a resistance-owned income zone, then assert exact money/HR deltas from the real `income_now` command. Record occupier/invader income potential separately so enemy-owned zones are proven excluded from player income. Treat the income report string as evidence only.
  - If an earlier debug stage calls player support commands, clear or cancel player-requested support requests and reset their cooldown fields before Phase 19 support smoke helpers.
  - Otherwise a valid support smoke command can fail for a cooldown created by the same one-button debug run.
  - When a debug suite intentionally tests several support types in one run, clear/cancel the previous player support request before each support-type probe. Then assert the newly created `HST_SupportRequestState` fields (`m_eType`, `m_sFactionKey`, target zone/position, ETA, money cost, queued/active status) instead of relying on the command text.
  - Support cancellation probes should capture the cancellation status/resolution before cleanup, then run player-support cleanup and assert no queued/active player support or cooldown remains. A cancelled row can remain as history, but it must not poison later support requests in the same debug run.
  - For QRF/search player support certification, push the debug-created request to the inbound physicalization window and tick `HST_SupportRequestService` instead of calling abstract completion directly. Assert remaining ETA decreases, runtime status advances, `m_bPhysicalized` flips, `m_sGroupId` is populated, and the linked active group has a support runtime status. Then sample `HST_PhysicalWarService.UpdateRoutedActiveGroupsNow()` across repeated route windows and report sample count, movement count, distance-closure count, max movement, max distance closed, and last-observed/stall history. After controlled arrival, a debug-owned support group can be moved to a terminal runtime status to prove `physical_group_terminal` resolution through the real support tick; keep natural support contact/combat as a separate gap until sampled without forcing the terminal state. Remove/cancel the debug support group/request before the post-case leak probe.
  - Snapshot the linked support marker immediately after the support command and marker rebuild, before the controlled runtime probe advances the request to resolved. Resolved support requests are intentionally omitted by `AddSupportRequestMarkers()`, so sampling marker state only after terminal resolution creates a false WARN even when the marker was published at request time.
  - Route-based physical probes should distinguish missing samples from sampled stalls. If a ground support or enemy-order group is sampled through the controlled route window and shows no movement, no distance closure, and no arrival status, record a `*.physical_stall_timeout` FAIL with timeout seconds, last-observed status, and sample history. Missing or insufficient samples can remain WARN/BLOCKED depending on prerequisite state, but a fully sampled no-progress window is a real failure.
  - Map-target support probes should assert the clicked/requested destination
    remains on `HST_SupportRequestState.m_vTargetPosition`, the linked active
    group keeps that same `m_vTargetPosition`, and deployment summary proves
    offset staging plus player/active-AI clearance before movement samples are
    judged.
  - Route-based physical probes should not fail movement or stall-timeout assertions when the sampled group is already within the target-distance tolerance. Distance closure is only meaningful while the group is still away from its objective; `distance_after <= 25m` is non-stall evidence even if the runtime status has not yet flipped to arrived.
  - Do not classify `spawn_pending_agents` or `convoy_seating_pending` as a route stall just because the debug runner advanced campaign seconds. Those states depend on real engine/callqueue population or animated boarding; keep them as WARN/pending evidence until the pending state clears, then fail only fully sampled no-progress windows.
  - Civilian aid debug probes should assert the exact clamped `RegisterIncident` result, not only direction. For the current aid command this means money -100, FIA support +12 clamped to 100, occupier support -6 clamped to 0, reputation +12 clamped to 100, wanted heat -2 floored at 0, and zone support clamped to the FIA-minus-occupier difference.
  - Civilian aid target selection must resolve an actual `HST_CivilianZoneState`/town zone near HQ. Reusing a generic support-zone selector can pick a depot/resource zone, where `RegisterIncident()` correctly returns false because civilian incidents only apply to town records.
  - Early garrison recruit/remove probes should arrange a non-active resistance-owned zone with real garrison slots, assert exact infantry/money/HR deltas through the public command paths, then restore temporary owner/active flags and remove an empty garrison record if the probe created one from nothing.
  - Capture garrison save-roundtrip evidence immediately after the recruit mutation and before the remove/cleanup command. Checking persistence only after cleanup proves the restore path, not the recruited garrison state the player actually needs to survive a save.
  - Phase 14 arsenal smoke should assert the `HST_ArsenalItemState` records directly. Finite-only mission props should create counted but locked records, threshold props should unlock at the configured count, blocked props should fail `CanDepositItem`, and raw visual/support assets should not create arsenal rows. Treat the report text as evidence, not as the pass/fail source.
  - Phase 15 garage/source smoke should prefix debug-created `HST_GarageVehicleState.m_sVehicleId` values during the one-button runner, assert stored cargo and source metadata directly, and include prefixed garage vehicles in cleanup/counting. Without that, source-vehicle smoke can leave persistent garage rows that the normal debug-prefix cleanup never sees.
  - Early garage/loadout certification should drive the real garage store/redeploy/capture commands and track the runtime vehicle id created by redeploy. Garage redeploy runtime ids are engine-derived rather than debug-prefixed, so the probe must capture or otherwise despawn the physical vehicle and remove the exact runtime/cargo records it created.
  - Workbench player prefabs can expose an inventory manager while still having no cargo insertion target for loose magazines. Physical saved-loadout probes should try a temporary storage carrier first; if neither loose cargo nor carrier insertion is possible, report the physical reflection branch as environment WARN while keeping serialized apply/ledger assertions authoritative.
  - Early command UI coverage can reuse the Phase 23 command/menu assertion helpers, but should still record an `early_mechanics` typed case so the phase 0-13 sweep is not represented only by a legacy text wrapper.
  - Phase 16 garrison/training smoke should capture before/after observations around the real recruitment/training service calls. Assert the selected zone is resistance-owned/inactive, the abstract garrison record changed by the requested infantry within slot capacity, zero-cost smoke actions did not spend money/HR, and training either increments once or reports a max-level no-mutation warning.
  - Phase 17 capture/counterattack smoke should remember the seeded capture zone and reuse it for force-progress and counterattack checks. Prefix only counterattack orders created during the debug run, assert ownership/progress/garrison/order/marker state directly, then physicalize the order through the real enemy commander/support path and sample repeated routed active-group movement/distance-closure/stall history. Before judging movement, resolve `spawn_pending_agents` through the active-group population path and record `*.physical_population`; route movement is not provable until durable agents are observable. Keep multi-wave/contact/arrival/resolution behavior as a separate gap until those transitions are sampled.
  - `HST_PhysicalWarService.UpdateRoutedActiveGroupsNow()` owns runtime group entity ensure, survivor refresh, and routed active-group updates. `UpdateZoneActivation()` should call it before zone activation/deactivation, and debug physical probes should call the route-only wrapper so support/QRF groups can advance without folding the target zone during measurement.
  - Support/QRF, enemy-order, convoy, and physical-combat debug probes should resolve `spawn_pending_agents` before judging route movement, readiness, or contact. Reuse the same active-group pending population path for primary proof: finalize native agents, queue one native `SpawnUnits()` retry, then try stock group member-slot population. Direct faction-infantry fallback may still run in the bounded runtime recovery window, but debug certification probes must record it as degraded/non-certifying evidence instead of using it to make route movement, convoy crew, or combat contact pass.
  - Phase 18/19 smoke assertions should verify debug-prefixed enemy order/support records, expected types, target zones/positions, resource cost fields, and resolve/ETA behavior. Stage 3 QRF/search support now samples repeated route advances, controlled ETA arrival, and controlled physical terminal resolution; keep Phase 19 enemy/support contact behavior as WARN/separate probes until those paths are sampled over time.
  - Phase 18 background-war coverage should seed resistance/occupier/invader POI ownership, top up enemy pools, run the real commander tick threshold, prefix the resulting orders, and assert both occupier and invader order evidence. This proves one controlled command-selection tick; it does not replace long-window escalation or physical advance sampling.
  - Phase 18 background-war commander probes must resolve pre-existing open enemy orders before measuring new order creation. The commander intentionally skips duplicate active orders for a faction/target-zone pair, so stale Phase 17/18 orders can otherwise hide one faction's expected controlled order.
  - Phase 18 background-war probes must not accept or own Petros attack orders. Keep the fixture at ordinary background-war pressure, select POIs outside the HQ threat radius, assert `background_war.commander_tick.no_petros_attack`, and immediately abort any unexpected Petros order so only Phase 22 can create the dynamic Defend Petros mission chain.
  - Phase 24 escalation pressure should reuse the Phase 18 background-war seed, then run separate low/mid/high profiles with exact enemy pool baselines. Capture pool totals before the resource tick, after `HST_EnemyDirectorService.TickResources()`, and after `HST_EnemyCommanderService.Tick()` so resource-income scaling is not confused with commander spending. Resolve open orders between profiles to avoid duplicate-order guards, prefix newly created orders/support/groups, and activate newly created physicalizable order target zones before the physicalization tick so `ShouldPhysicalizeOrder()` has the render-bubble condition the assertion claims to test. Keep group movement as WARN/evidence unless movement is sampled.
  - Phase 24 can cover a short repeated background-war window by running several normal resource/commander tick intervals after seeding the same POI ownership graph. Resolve debug-created open orders between cycles so duplicate-order guards do not hide later commander decisions, retag every created order with the active run prefix, and assert repeated resource ticks, repeated commander ticks, created order counts, and no open prefixed orders after the controlled cycle cleanup. This is stronger than a single commander tick, but it is still not a substitute for an extended autonomous soak across many POIs.
  - Phase 24 pacing seeds are economy/end-state probes, not HQ-threat probes. Reset HQ knowledge/threat/last attack state during those debug seeds so a prior Phase 22 Petros-defense sample cannot spawn a new Defend Petros mission and contaminate pacing cleanup assertions.
  - Aggression decay certification should set a known aggression value on every non-resistance pool, reset the aggression accumulator, tick exactly one configured decay interval, and assert the total decrease equals `decayAmount * enemyPoolCount` without underflow.
  - Phase 20 town support smoke should drive the real `HST_CivilianService.RegisterIncident()` path in both directions, then restore the seeded town state before the runner continues. Assert FIA/occupier support direction, `HST_ZoneState.m_iSupport == clamp(FIA - occupier)`, support/reputation/heat bounds, incident metadata, support-only owner flips, zone marker model owner/color/style reflection, and cleanup restore. With a preset supplied, town incidents flip from enemy ownership to resistance when FIA support is at least 65, FIA leads occupier support by at least 15, and wanted heat is at most 5; resistance-owned towns flip back to an enemy faction when occupier support is at least 65 with a 15-point lead or wanted heat is at least 15 while occupier support exceeds FIA support. The debug probe must restore owner, capture progress, support, heat, incident/security metadata, undercover restriction state, and the rebuilt zone marker model after sampling both directions.
  - Phase 20 wanted-heat smoke should include a copied-state heat-decay probe through `HST_CivilianService.Tick()`. Use a temporary `HST_CampaignState` with copied town/player records, advance it by `HST_CivilianService.HEAT_DECAY_SECONDS`, and assert town heat decreases by exactly one while the town incident timestamp advances. The current player undercover heat behavior cools by one eligible civilian-service tick once the player is not still compromised; record that current behavior directly instead of silently treating it as the same town-window timer. Assert the live campaign town/player records remain unchanged after the copied-state probe. This certifies heat decay logic, not a broader civilian reaction state machine.
  - Phase 20 civilian population smoke should use the scoped `HST_CivilianService.UpdatePhysicalTownPopulationForZone()` helper for a selected inactive town. The global population tick can spawn or clean other active towns, which makes a one-case assertion harder to cleanly restore. Assert live CIV character/vehicle counts, CIV faction tags, spawn radius, spawn-failure delta, bounded movement samples from runtime spawn positions, and cleanup of runtime entities plus transient runtime-vehicle records.
  - A scoped civilian population update returning `false` is not by itself a failed activation. If the runtime town record exists and live CIV runtime entities were spawned, treat the population as active and record the no-delta update as evidence.
  - Civilian population runtime currently owns spawn/cleanup/diagnostics, not a service-level waypoint or reaction state machine. A debug movement probe can sample `IEntity.GetOrigin()` against the stored spawn position and report moved-count/max-distance/timeout evidence, but static civilians should remain WARN evidence until a real civilian behavior contract exists.
  - Phase 21 undercover apply smoke needs a controlled eligibility fixture. Normal player requests should validate the live entity, but the admin smoke can use `BuildUndercoverEligibility(..., checkCurrentEntity = false)` after seeding clean town/player heat so Workbench's military player prefab does not make the apply path fail before exposure scans are tested. Reset transient player heat, compromise timeout, detection source/score, last scan failures, and cached eligibility reasons before calling the real request; stale roadblock/police evidence from earlier smoke steps should not poison the apply fixture.
  - Phase 22 HQ/Defend Petros smoke should assert the state chain, not only the command text: seeded HQ knowledge/threat reasons, a debug-prefixed Petros attack order, a debug-prefixed dynamic defense mission/objective/task, active defense markers, linked support request evidence, repeated routed attacker-group movement/distance-closure/stall history, admin-success resolution, Petros death/runtime-object clearing, and debug recovery. Prefix the defense mission before objective/task creation so cleanup can remove the whole record chain; retag a later physical support request before it spawns a group. Keep multi-wave/contact/arrival/defense pressure as WARN until those transitions are sampled over time.
  - The Defend Petros attacker marker is a QRF-style tactical marker, but its backing state is the attacker active group created from the linked Petros attack support request. Marker cleanup/backing audits must accept a non-terminal active group for that marker, and marker generation must stop publishing it once the group is eliminated, folded, spawn-failed, despawned, or deleted.
  - Petros attack orders are attacks on the resistance base, not the nearby strategic zone used as routing/source context. Keep the zone ID for support source lookup and duplicate-order guards, but set `m_vTargetPosition` to `m_vPetrosPosition`/`m_vHQPosition`, make notifications/reports/support markers describe the target as `HQ/Petros`, and assert order/support/group/mission/objective/task positions against the base position instead of the zone center.
  - Petros attack support active groups must keep their HQ/Petros target after physicalization. Generic static active groups can have routes flattened to their current position after arrival, but Defend Petros attackers still need `m_vTargetPosition` to remain the base position for movement, marker, and Phase 22 proof. Tag those support groups separately and skip static route normalization for them.
  - Campaign victory/loss should terminally resolve an active Defend Petros chain without awarding mission rewards: close the dynamic defense mission/task/objective, resolve the Petros order, cancel linked support, fold the attacker group, and clear the active flag so terminal campaign state does not preserve a live base-defense mission. Coordinator ownership/capture wrappers must run this cleanup after calling strategic zone-owner helpers because those helpers can apply campaign end internally.
  - Petros attack support has a stricter HQ standoff than generic support staging. Resolve staged/arrived positions with a Petros-specific margin and reject fallback positions against the Petros attack standoff, not just the smaller HQ safe radius, or physicalization can fail with `physicalize_failed_hq_standoff` before an attacker group exists.
  - Passive HQ awareness should not immediately create base-defense pressure. Keep HQ enemy-activity, civilian-heat, and commander HQ-pressure radii as named constants, do not score HQ pressure while HQ knowledge is zero, and require non-zero knowledge before opportunistic Petros attacks can be selected.
  - Global enemy aggression can inform HQ threat reports, but it must not reveal HQ location by itself. Passive HQ knowledge gain should use local HQ exposure only, stay cooldown-gated, and be capped per scan so placing HQ far from contact does not still race into Defend Petros.
  - Petros attack support now uses a closer dedicated staging band: the hard minimum is 760m from HQ, the normal requested standoff starts at 850m, and the max accepted staging distance is 1120m. Full Campaign Debug should assert both support placement and the linked Phase 22 attacker source stay inside that band.
  - Routed response groups should move faster than ordinary garrison spawns. Apply `AIGroupMovementComponent.SetGroupCharactersWantedMovementType(EMovementType.RUN)` only to support/QRF/Petros attack active groups during route-waypoint assignment, record `response_run` in `m_sSpawnFallbackMode`, and do not apply that token or movement override to normal garrison population.
  - Any Phase 22 debug command that creates or reuses a Petros attack order must apply the current campaign-debug order prefix before `EnsureDefendPetrosMissionForOrder()` links state. `queue attack` and `start defense` can create orders through different paths; both must retag or cleanup/report assertions will see unprefixed `order_<faction>_<time>_<index>` records.
  - Phase 23 UI/marker smoke should assert command/menu and marker state directly: no `missing visible command:` or `missing dispatch:` detail rows, admin menu strings for campaign-debug and Phase-23 controls, every `hst_zone_<zoneId>` marker model entry with linked ID, owner, color, style, and position matching the current `HST_ZoneState`, HQ/mission/support/QRF marker coverage from `HST_CampaignState.m_aMapMarkers`, and marker/backing-state consistency via the same helpers used by post-case cleanup. The native marker report should rebuild markers before reporting, then assert native eligible/published/skipped/failed/pending counters and tracked static handle liveness through structured service getters instead of string parsing. Keep `HST_MapMarkerService` and `HST_CampaignMapMarkerDirector` native marker budgets synchronized; the curated map currently needs more than 96 native-eligible records. Failed-action samples should snapshot state before and after invalid commands so they prove reasoned failure with no campaign mutation. Native map-marker manager absence should be explicit WARN/report evidence; visual widget inspection still requires a separate UI/render probe.
  - Marker/backing consistency probes should force `RefreshCampaignMarkers()` before counting missing backing markers. Mission/support/QRF creation often sets marker-refresh-needed flags and relies on the normal tick throttle, so checking `m_aMapMarkers` first can produce stale post-case cleanup WARNs even though the marker service can rebuild the correct backing records immediately.
  - A mission can be correctly represented by a specific objective/asset marker instead of its generic mission marker. `HST_MapMarkerService.AddMissionMarkers()` suppresses the base `mission.m_sMarkerId` when a visible mission asset or objective marker exists, so debug marker/backing verification must accept any visible marker linked to the mission instance.
  - Final cleanup should treat raw marker count deltas as metrics only. A full debug run can legitimately rebuild HQ, zone, mission, support, or QRF markers above the run-start count; cleanup pass/fail should be based on prefixed debug records, visible linked markers without backing state, and active backing records without markers after a forced marker refresh.
  - Zone marker position assertions should compare X/Z placement with a small tolerance, not exact 3D vector equality. `HST_MapMarkerService.AddMarker()` resolves marker presentation height through `ResolveGroundPosition()`, so marker Y can differ from `HST_ZoneState.m_vPosition[1]` even when the marker is correctly placed on the map.

  - Campaign-debug generic command outputs and read-only report snapshots should use typed `action.*` and `observation.*` cases instead of the `legacy.*` text wrapper. These cases should record the full text as evidence and assert non-empty output so generic rows appear in matrices without claiming feature-specific physical coverage. Mutating command/action rows should keep the normal failure-string classifier; read-only observation rows should use `IsCampaignDebugObservationReportHealthy()` and only fail on empty text, access denial, missing services/state, or missing visible-command/dispatch coverage. Historical `failed:`/`FAIL` text inside an otherwise available diagnostic report is evidence for that report, not proof that the observation step itself failed.

- A one-button debug run can cover in-process Phase 25 soak checks, but not external session conditions.
  - An in-memory `HST_CampaignSaveData` restore is useful certification evidence for copied state shape, but it is not a substitute for a process restart, second-client reconnect, or long soak.
  - Field-vehicle persistence smoke can seed a restore-eligible `field_vehicle` runtime record and prove it survives `HST_CampaignSaveData` capture/restore exactly once. Keep this labeled as record-level restore evidence; physical vehicle respawn through `HST_LootService.RestorePersistentFieldVehicles()` still needs real world/process restore evidence before it can be counted as complete.
  - Report real restart-after-each-primitive, second-client join/reconnect, and two-hour endurance as explicit WARN/manual gaps instead of silently treating them as covered.
  - Keep the rest of the Phase 25 summary tied to actual counters from the sequenced run: early phase steps, mission definitions, Phase 14-24 smoke steps, and aggregate pass/warn/fail totals.

- Generated alpha content must not retain invalid anchor positions.
  - `HST_GeneratedContentService` should repair generated mission sites to accepted dry/road/vehicle-safe positions before insertion. If a zone anchor itself is unusable, search around linked zones before falling back to the original position.
  - Route validation should attempt to repair unsafe waypoints with a wider road/large-vehicle search before marking the route invalid; otherwise coastal or offshore source anchors can poison the whole generated-content smoke case even when nearby linked land anchors exist.

- A destructive full-campaign runner that calls member/commander APIs must normalize the clicking admin's authority for the run.
  - Admin status alone does not imply commander status. Temporarily make the actor member/commander while the runner executes, then restore the previous commander identity on completion.
  - This avoids false failures in commander-gated systems such as HQ rebuild, income, training, and support requests.
  - Bootstrap evidence should record the debug actor's backend UUID, SteamID64, and admin grant reason. Treat missing SteamID64 settings proof as WARN when the actor is already admin by another path, not PASS.

- Player identity for h-istasi state and admin configuration use different durable IDs.
  - Dedicated server admin/menu tests must first prove the packaged addon is current. The server log should print `h-istasi boot | authority build ...` from `HST_CampaignCoordinatorComponent`, and the client local-ready menu line should include the command-menu build stamp. If those current stamps are missing, the run is using a stale `ArmaReforger/addons/.../data.pak` package even if the Workbench repository has the fix.
  - `PlayerId=2` is only a per-session connection id; it can change on reconnect and should not be stored in `membership.adminIdentityIds`.
  - `membership.adminIdentityIds` should only contain raw 17-digit SteamID64 values. Do not store backend UUIDs, `workbench_player_N`, prefixed aliases, or session player ids there.
  - On the server, `GetGame().GetBackendApi().GetPlayerPlatformId(playerId)` is the intended script-side platform-id source for matching the SteamID64 value and should be proved in h-istasi grant logs. If that path does not match, log the backend UUID, platform-id candidate, and configured admin count before assuming the settings token is wrong.
  - `GetGame().GetBackendApi().GetPlayerIdentityId(playerId)` returns the backend UUID shown in backend/network logs after authentication. Use that UUID for internal persistent player/member/commander state, with `workbench_player_N` only as a local/early bootstrap placeholder, not as an admin fallback.
  - If a player was registered before the backend identity was available, migrate the placeholder record and rewrite commander/loadout/undercover owner references once the UUID resolves.
  - Spawn-sweep registration must route through `HST_CampaignCoordinatorComponent.RegisterConnectedPlayer()` or otherwise apply the same runtime admin grant logic. Direct calls to `HST_PlayerLifecycleService.RegisterConnectedPlayer()` can create a live campaign player before the SteamID64 admin grant has been applied.
  - Server-host admin lists can be bridged through the session player id after connection: use `SCR_PlayerListedAdminManagerComponent.GetInstance().IsPlayerOnAdminList(playerId)` or `BackendApi.IsListedServerAdmin(playerId)` for runtime admin grants, but do not persist the session id as the durable h-istasi identity. This is a separate native-server admin source, not a third `membership.adminIdentityIds` token type.
  - Runtime settings parsing must handle `adminIdentityIds` as either a compact one-line JSON array or a pretty-printed multi-line JSON array. The profile file can be reformatted by external tools; a line-only parser will silently clear the configured admin list.

- The `I` key is already present in the base input config as native action `PlayerMenuInvite` under `PlayerMenuContext`.
  - The h-istasi command menu can keep its custom `HST_CommandMenu` action/binding, but the client component should also activate `PlayerMenuContext`, activate `PlayerMenuInvite`, listen for `PlayerMenuInvite`, and include that native action state in input heartbeat logs.
  - During initial HQ placement the setup map owns input and clears `KC_I`; finish HQ placement first, then `I` should open the command menu. A setup-active refusal is different from a broken command-menu binding.
  - Petros/HQ contextual actions call the same menu renderer; log those opens as `contextual action` so they are not mistaken for successful `I` key opens.
  - Treat `h-istasi menu | native I action input detected source=PlayerMenuInvite ...` followed by `h-istasi menu debug | opened via native PlayerMenuInvite` as proof that the native `I` path reached and opened the HST command menu. If a player reports the key still failing, first confirm a newer client log exists and check for setup/UI-root/debounce refusal lines before changing input bindings.
  - Selectable command prompts should be data-driven by the menu payload, not inferred from already-rendered row text. The commander-transfer member control uses one visible `member_promote_commander_choose` action carrying sanitized SteamID64/name choices, opens a fixed action-choice modal, and dispatches the existing `member_promote_commander` command only after the clicked choice supplies the selected SteamID64.
  - Campaign-debug UI coverage should require the chooser command in `BuildCommandCoverageReport()` and inspect the actual Members/Admin payloads, not just summary strings. Expected proof is one `Transfer commander` row using `member_promote_commander_choose`, no direct `member_promote_commander` action rows in Members, and an Admin `Force myself commander` row using `admin_force_self_commander`.

- Disabling Game Master budgets disables placement caps, not all native budget accounting.
  - The native `SCR_EditableEntityCoreBudgetSetting.SubtractFromBudget(int)` path logs `GM Budget got clamped at some point!` when a queued delete/free operation subtracts more than the tracked current budget.
  - h-istasi should keep `SCR_BudgetEditorComponent.IsBudgetCapEnabled()` false for managed GM budgets while disabled, prefill managed budget caps/current accounting to disabled-mode headroom, and guard `SCR_EditableEntityCoreBudgetSetting.SubtractFromBudget()` before vanilla subtracts. Post-update repair alone is not enough; native queued deletion can still reach the clamp path before diagnostics make the run understandable.
  - Keep `SCR_EditableEntityCore.Event_OnEntityBudgetUpdatedPerEntity` as a last-resort deficit repair hook, but restore full disabled-mode headroom there instead of only adding the single negative delta back.
  - Treat `preSubtractRepairs` / `deficitCorrections` in `HistasiBuildGameMasterBudgetDiagnostics()` plus at most one `restored disabled-budget headroom before native subtract` or `corrected first disabled-budget deficit` log line as proof that the disabled-budget shim is active without accepting per-frame correction spam.
  - Campaign-debug preflight should record `preflight.gm_budget.*` assertions showing settings/runtime/game-mode agreement, budget editor availability, cap-enabled state, managed-budget headroom, original cap counts, and deficit-handler registration. A full run with `gameMasterBudgetsEnabled=false` should prove placement caps are disabled and the deficit correction hook is active before accepting any GM-budget clamp investigation as closed.
  - Policy diagnostics alone are not enough for disabled-budget certification. Preflight should also spawn a temporary debug-tagged runtime prop near HQ through `HST_WorldPositionService.SpawnPrefab`, immediately request cleanup, and record `preflight.gm_budget.disabled_spawn_probe` so the live placement/spawn path is exercised while GM budgets are disabled.

- HQ/Petros runtime proof must require exactly one real live Petros character near the HQ Petros slot; AIGroup attachment is diagnostic evidence.
  - A Petros entity handle is not sufficient unless the character is alive, prepared, at the HQ Petros position, and exposed through the HQ service's tracked handle. Dedicated-server logs can show `Petros character runtime was removed after spawn`, so disappearance should still trigger a respawn.
  - A freshly spawned `ChimeraCharacter` can exist before `GetCharacterController()` is available. HQ runtime liveness must not classify that state as dead; use controller life state when present and otherwise fall through to damage-state/entity-exists proof so Petros is not deleted during initialization.
  - Re-run `PreparePetrosEntity()` on a tracked Petros during HQ runtime refresh. The first prepare can happen before the controller exists, so the later refresh is what applies stationary movement/weapon locks once the controller becomes available.
  - Dedicated-server runtime tests showed direct Petros character spawning can create a character that is removed before the next HQ tick. The primary path must spawn `HST_PetrosGroup.et`, set max units to one, call `SpawnUnits()`, and resolve the live Petros character from the group's controlled agent before the HQ runtime proof passes. Keep forced character spawning only as fallback/diagnostic evidence.
  - After spawn or reattach, call `EnsurePetrosAIGroup()` and report `BuildPetrosAIGroupDebugSummary()` evidence, but do not delete a living Petros only because the optional `SCR_AIGroup` link is missing or later drops out of service tracking.
  - `AIAgent.GetParentGroup()` returns base `AIGroup`. When Petros diagnostics need to compare against the tracked `SCR_AIGroup`, keep the parent value as `AIGroup` and compare it to the tracked group; Workbench rejects `SCR_AIGroup.Cast(agent.GetParentGroup())` on this native return path.
  - Petros AIGroup attachment is only a reattach fallback for an already living world Petros. The primary proof path is the inverse: `HST_PetrosGroup.et` owns the Petros unit slot, `SpawnUnits()` creates the character, and `AIAgent.GetParentGroup()` proves the character belongs to the tracked group.
  - Empty `SCR_AIGroup` roots can queue native deletion during init before script code gets a chance to call `SetDeleteWhenEmpty(false)`. Petros must use the HST-owned `HST_PetrosGroup.et` prefab with `m_bSpawnImmediately 0`, `m_bDeleteWhenEmpty 0`, FIA base-group inheritance, and a single Petros unit slot; do not attach him to raw `Group_Base.et` and then try to disable deletion after spawn.
  - The campaign-debug HQ case should include an explicit `hq.petros.ai_group` assertion as WARN-level diagnostic evidence. The hard HQ runtime proof is the live tracked Petros character plus cache/arsenal/tent/spawn-point handles.
  - Do not let `m_bHQRuntimeObjectsSpawned` go true from raw handle presence alone. The final HQ runtime flag should require tracked runtime objects, a usable HQ arsenal entity, and exactly-one living Petros world proof; otherwise later debug phases can treat a visually spawned but unusable HQ as certified.
  - When campaign state is available, Petros tracking should be state-aware: exactly one living prepared/world-scan Petros near the HQ slot must exist, and that entity should be reattached to `m_PetrosEntity` before spawning another Petros. A cached handle alone must not mask a missing or duplicated world Petros, and a stale cached handle must not keep the HQ runtime flag false when the unique living world Petros is proven.
  - HQ cache, arsenal, tent, and spawn-point handles should be reattached from unique nearby world entities before spawning replacements. `m_bHQRuntimeObjectsSpawned` should require unique world proof for Petros plus all four non-Petros objects; stale service handles are not enough proof, and handle loss must not create duplicates.
  - During Petros-driven HQ relocation, `HST_HQService` needs an explicit relocation mode so normal HQ runtime refresh keeps the tracked living Petros handle prepared but does not snap him back to the HQ slot or lock movement. Any manual HQ asset rebuild or runtime clear should cancel that relocation mode and send the owner-local action state back to false before deleting/rebuilding Petros.
  - Petros follow-in-vehicle behavior can use `SCR_CompartmentAccessComponent` directly: resolve the player's current vehicle with `CompartmentAccessComponent.GetVehicleIn()`, choose an accessible unoccupied cargo `BaseCompartmentSlot`, call `GetInVehicle()`/`MoveInVehicle()` on Petros' access component, and force a teleport get-out when the target leaves the vehicle. This keeps Petros with the followed player without claiming the vehicle.

- Active AI groups cannot wait until the final retry before proving live agents.
  - Native group prefabs can spawn an `SCR_AIGroup` with zero countable agents for several ticks on a dedicated server.
  - Queue one native `SpawnUnits()` retry, then force direct faction infantry fallback before the final population grace attempt. The fallback must use the active group's faction key and debug output must prove expected faction, live count, and zero mismatches for the spawned members.
  - Direct faction-infantry fallback members should only count after `SCR_AIGroup.AddAgentFromControlledEntity(member)` or its fallback attach path proves the spawned member's `AIAgent.GetParentGroup()` is the intended runtime group. Restamp the entity faction after attachment so group-side native initialization cannot leave a US/USSR fallback member affiliated as FIA.
  - Group prefab selection must reject wrong-faction catalog paths for the built-in FIA/US/USSR factions. US groups must come from BLUFOR/`Group_US_`, USSR groups from OPFOR/`Group_USSR_`, and FIA groups from INDFOR/`Group_FIA_`; changing only the runtime faction stamp is not enough proof because uniforms/loadouts still come from the prefab catalog.
  - Campaign-debug preflight should also assert faction catalog identity before spawning anything: FIA infantry/groups from INDFOR/FIA, US from BLUFOR/US, and USSR from OPFOR/USSR. This catches a template/pool regression even if runtime faction stamping later masks it.
  - Runtime faction audits should report the selected prefab path, the `SCR_AIGroup` root faction, and all controlled member/vehicle faction components. A group root stuck as FIA with expected US/USSR is a mismatch even before members are countable.
  - Post-case and final cleanup should run the physical-war runtime faction audit. `post_cleanup.runtime_factions` / `cleanup.runtime_factions` must report zero mismatches across live runtime group members and runtime vehicles, otherwise the run has not disproved the "everything spawned as FIA" failure.
  - Runtime faction cleanup should not classify a terminal active group with no live controlled members or live vehicle as a faction mismatch just because its empty `SCR_AIGroup` root is still tracked. Report skipped terminal-empty roots separately, keep pending live-count grace as evidence, and reserve faction mismatches for live members/vehicles, wrong-faction roots, or infantry groups with zero live controlled members after grace.

- Full-campaign debug report classification must not scan stale aggregate text as if it belonged to the current action.
  - Mission-sweep runtime checks should inspect the selected mission instance, then append selected convoy diagnostics only for that mission. Global mission runtime reports include completed/failed historical mission records and can poison every later mission with old `failed:` text.
  - Diagnostic report steps should be judged by report availability/admin errors, not by generic action failure substrings. Summaries such as `missing dispatch 0`, expected Defend Petros failure text, or historical support/order failure reasons are useful diagnostics, not proof that the current report step failed.
  - UI coverage reports should only fail on explicit detail rows such as `missing visible command:` or `missing dispatch:`, not on the zero-count summary labels.
  - Baseline persistence diagnostics should distinguish a broken save path from an unavailable native backend with a working profile fallback. In Workbench, `PersistenceSystem unavailable` plus `profile fallback 1` is a warning-level environment limitation; failed fallback save/load/read or `profile fallback false` is a real failure.
  - Keep the baseline persistence check focused on `HST_PersistenceService.BuildPersistenceReport()`. Do not append the unseeded persistence-smoke report there; the dedicated seeded persistence smoke steps own that validation later in the run.

- Convoy mission runtime should not fall back to a generic mission prop when convoy vehicle asset planning fails.
  - A convoy with `spawned 1` and `vehicle asset count 0` is misleading: the physical convoy did not exist even though the generic runtime prop spawned.
  - Keep the convoy unspawned, preserve `m_sRuntimeFailureReason`, and log the road/destination/slot-plan reason so the next debug run shows whether the blocker is destination road resolution, the 2000-5000m band, full-column slot probing, or vehicle footprint checks.
  - Convoy start planning should keep the 2000-5000m road-resolved band, but synthetic radial probes are not enough near sparse road/coastal regions. If random band probes miss the road network, try known generated route anchors and then generated route segment samples across the campaign state as fallback start candidates. Every fallback candidate must still run the same road, clearance, band, and full-column slot checks before creating vehicle assets.
  - Mission-sweep convoy runtime assertions should require the full planned convoy vehicle count, never only one placeholder asset. Use `max(3, mission.m_iRequiredVehicleCount)` as the minimum expected `convoy_vehicle` asset count.
  - Convoy vehicle spawn and AI crew population are asynchronous. If an AIGroup initially has zero agents, preserve `spawn_pending_agents` while the convoy vehicle is spawned; otherwise the delayed population callback will skip the group and the convoy health check can fail immediately with `Convoy could not spawn three crewed vehicles.` even though the agents were still inside the population grace window.
  - Faction-aware convoy vehicle, crew, zone infantry, zone vehicle, and QRF group selection depends on `HST_CampaignPreset`. Thread the preset through nested runtime helpers such as routed-group sweeps, zone activation, pending QRF spawning, convoy group spawning, and vehicle spawning; Enforce does not capture an outer `preset` variable when the helper signature does not include it.
  - When the delayed AIGroup population callback does find agents for a convoy crew group, immediately retry the convoy vehicle binding path. The normal spawn path may already have both crew and vehicle runtime handles, so a handle-missing respawn check will not necessarily revisit seating by itself.
  - Before any convoy bind attempt, move unseated living crew to safe ground around the vehicle. Dedicated-server staging can leave newly populated crew at the group spawn offset while the vehicle is elsewhere, and repeated seat requests from that offset may never produce a confirmed driver.
  - Convoy crew control has two async windows: population and vehicle seating. Dedicated-server logs can show direct fallback crew agents populated, followed by `convoy_seating_pending` while Enfusion animates boarding or driver assignment. During a short seating grace, do not treat `CountAliveRuntimeCrewAgents() == 0` as combat contact or `convoy_eliminated`; keep contact/survivor/readiness assertions pending/WARN until the grace expires.
  - Direct fallback infantry can coexist with a native `SCR_AIGroup` shell under the same active-group ID. Runtime live-count helpers must collect unique living controlled entities from the native group plus separately registered direct members; otherwise an empty native shell masks the fallback crew and convoy readiness reads as zero living crew.
  - Convoy debug readiness assertions should respect `m_bPendingGrace`: missing seated drivers, routes, or waypoints during the staging grace are WARN evidence for async crew boarding, not hard failures. Apply the same rule to per-group `convoy.group_waypoints.*` assertions when the active group is still in `convoy_seating_pending`; after the grace expires, the same missing readiness fields are real convoy failures.
  - Per-asset convoy physical assertions must also respect the same pending window. A missing `convoy.crew_entity.*` handle is WARN while `BuildMissionConvoyReadinessStatus()` reports pending grace or `IsConvoyCrewControlPending()` is true, then becomes FAIL after the population/seating window expires.
  - Generic convoy failure logs should include group context: vehicle asset count, attempted/spawned groups, pending control count, alive crew groups/count, runtime handle counts, and one sample group status/reason. The one-line warning should be enough to tell whether the failure was asset planning, pending AI population/seating, missing handles, no living crew, or vehicle binding.
  - A seated convoy driver and `AIGroup.AddWaypoint()` are not enough evidence that the engine vehicle-AI layer owns the spawned vehicle. Before assigning convoy route waypoints, resolve the vehicle's `SCR_AIVehicleUsageComponent`, require a valid pilotable usage type, and register it with the crew group's `SCR_AIGroupUtilityComponent.AddUsableVehicle()`. The debug suite should report this separately from waypoint count and movement distance.
  - Convoy completion must not infer `all crews eliminated` from missing runtime handles, pending population/seating control, or zero live count during the pending window. Count missing and pending crew groups separately and only complete on elimination when the crew runtime was observable or explicitly terminal.
  - A planned convoy crew count is not an observed living crew count. Keep `m_iLastSeenAliveCount` and survivor counters at zero until live crew is actually counted, and track whether crew was ever alive plus the maximum observed crew so failed population is reported as crew population failure instead of crew elimination.
  - A terminal convoy crew status is not proof by itself. `convoy_eliminated`/`eliminated` should only count toward convoy completion after the crew group has explicit live-crew history (`m_bEverHadLivingCrew` or `m_iMaxObservedCrewAlive`). Do not use preserved last-seen/survivor counters as historical proof; they are bookkeeping. Debug-controlled phase-chain probes may seed that history explicitly; real physical convoys must observe it.
  - A pre-contact crew-count drop is not physical contact proof. Before `convoy_contact`, zero live crew should trigger the convoy-specific repair path or remain `CREW_UNOBSERVED`/pending with explicit evidence; only player contact, destroyed/captured vehicle state, an already-contacted mission, or terminal asset state may make crew loss count as convoy contact/elimination.
  - Convoy reward/outcome application must repeat the same live-history proof. `HST_ConvoyOutcomeService` should not apply `convoy_complete`/`convoy_eliminated` crew rewards from mission event tokens unless matching `mission_convoy_` active groups are eliminated and each group has observed or explicitly seeded live-crew history.
  - Post-completion convoy asset preservation and UI actions must repeat the same live-history proof. Do not preserve or reopen convoy payload, captive, or vehicle interactions from `convoy_complete`/`convoy_secured_sent` tokens alone; pass campaign state into the mission-runtime/UI helpers and require the guarded crew outcome or matching eliminated groups with live-crew history.
  - A campaign-debug convoy physical probe should force a narrow `EnsureMissionConvoyRuntimeNow()` service pass for that mission before asserting physical readiness. Otherwise the probe can sample stale mission state immediately before the normal physical-war tick would have created or repaired runtime handles.
  - Campaign-debug convoy physical case ids should include the runner label/window. The same convoy mission can be sampled during movement/readiness and later during contact; those are distinct evidence windows and should not produce duplicate case ids.
  - Convoy readiness reports must distinguish planned vehicle assets from active/unresolved vehicle assets. A failed or terminal convoy can have planned assets and zero active assets; reporting that as `vehicle assets 0` hides the real failure. Use planned counts for asset-planning assertions and active counts for spawned-vehicle, crew, route, waypoint, faction, and movement thresholds.
  - Convoy setup should request `SCR_CompartmentAccessComponent.MoveInVehicle()` first, because it routes the seat request to the character owner and locks the slot for that frame. Keep the direct `GetInVehicle(slotOwner, slot, true, ...)` call as a fallback, then rescan compartments before judging driver availability. If a request is accepted but no driver is visible in the same slice, preserve explicit `convoy_seating_pending` evidence instead of reporting a hard no-driver failure.

- Runtime tracking arrays must stay lockstep.
  - When maintaining parallel arrays such as runtime group ids/entities, runtime vehicle group ids/entities, pending population ids/status/group/state, or blocked vehicle zone ids/reasons, remove sibling arrays at the target index before removing the key/id array. Removing the key first can make `i < sibling.Count()` false for the final element and leave orphan handles or reasons shifted out of alignment.

- Campaign-debug cleanup assertions should test ownership/backing, not raw count drift.
  - A long full-profile run can legitimately leave backed, non-debug active groups created by normal campaign services. Treat total active-group count as a metric/context line. The hard cleanup invariant is no `hst_debug_` prefixed state and no active group lacking zone, mission, support, enemy-order, or QRF backing.
  - Before post-case or final cleanup audits runtime factions, force a primary-path pending-population drain for active groups still in `spawn_pending_agents`. Use the recorded pending requested status when available, and keep direct faction-infantry fallback out of this cleanup drain so `runtime_group_primary_spawn` remains meaningful.
  - Runtime faction cleanup should skip groups that are still explicitly pending population or AIWorld-deferred after the drain. Those rows belong to `runtime_group_population_settled`; counting the same empty group root as both a population blocker and a faction mismatch hides the real cleanup cause.

- Terminal campaign phases need special runner handling.
  - If `EOnFrame` returns early for `HST_CAMPAIGN_WON` or `HST_CAMPAIGN_LOST`, tick the debug runner before returning or a forced victory/loss step can strand the sequence.
  - Do not auto-repair won/lost back to active before reading the campaign-end report steps. Let the forced loss/victory helpers reset state when they need to set up their own terminal scenario.
  - Phase 24 post-end checks should snapshot state immediately after forced victory/loss, then compare the following delayed report step. Assert elapsed seconds, mission/objective/asset/support/order/group/runtime-vehicle counts, money, HR, and income timer are unchanged to prove the terminal frame branch skipped normal campaign services.
  - After the debug runner purges native/static markers or removes debug-prefixed state, force an immediate player-marker refresh/reconcile before reporting completion. The player marker service can legitimately clear dynamic markers during transient no-player respawn slices, but completion should leave a live desired/tracked player marker for manual map inspection. Bypass the cached player-marker reconcile signature for this completion refresh and record desired/tracked/live counts in the run artifact so a missing post-run map marker is visible in structured evidence.
  - `m_bHQRuntimeObjectsSpawned` describes live entity handles, not durable campaign data. Save capture/restore should force it false, and the terminal-frame branch should still call the HQ runtime-object rebuild so a persisted won/lost debug save can show Petros, cache, arsenal, tent, and spawn point without resuming normal campaign services.
  - HQ/Petros access needs an object fallback, not only the Petros character or the `I` key. The July 2026 server/client logs showed the HQ arsenal survived and opened the loadout editor while Petros/runtime command access was missing, so keep an HQ menu user action on the arsenal and keep the arsenal action filter aware of all h-istasi custom actions.

- Aggregate debug results should separate action assertions from diagnostic reports.
  - Score mutation/test commands through a shared failure classifier (`failed:`, server/admin required, not-ready, `FAIL` smoke output).
  - Log read-only reports as INFO unless the report itself is empty or indicates a hard service failure.
  - Intentional negative-path samples, such as the Phase 23 failed-action sample, should count as PASS only when the expected commands fail with explicit reason text and before/after mutation snapshots stay equal. The negative path itself is not warning-worthy; missing failure text or silent state mutation is a real FAIL.
  - Phase 20/21 undercover smoke should assert `HST_PlayerUndercoverState` directly. Useful fields are `m_eStatus`, `m_iWantedHeat`, `m_bUndercoverRequested`, `m_bUndercoverApplied`, `m_sAppliedMode`, `m_sLastDetectionSource`, roadblock/police scan counts, and last scan failure booleans.

## AI And Spawning

- Native `SCR_FreeSpawnData` requests require a fully available player controller.
- Runtime faction repair must cover every registered runtime entity for an active group, not just `GetRuntimeCrewGroupEntity()`. Direct infantry fallback stores the AIGroup root and individual character handles in parallel arrays; an audit that checks those direct handles can still fail if only the root group was repaired.
- Faction proof should include the actual spawned prefab evidence in addition to `FactionAffiliationComponent`/`SCR_AIGroup.GetFactionName()` results. A correct faction key does not prove that the visible character/equipment prefab came from the US or USSR catalog.
- Game Master group cards can expose the group root's entity affiliation separately from `SCR_AIGroup.GetFactionName()`. Runtime group stamping must set both the native `SCR_AIGroup` faction and the root `FactionAffiliationComponent` when that component exists, and debug visual evidence should print both `groupFaction` and `rootFaction`.
- Do not use a faction-defaulted empty `SCR_AIGroup` prefab as a universal direct-infantry fallback root. If the fallback root is authored as FIA, US/USSR groups can look or classify as FIA in Game Master even when the active-group state expected an enemy faction. Use faction-specific empty group roots, and make runtime `SCR_AIGroup.SetFaction()` calls idempotent so survivor/audit ticks do not repeatedly reset the same group container.
- Faction-specific empty runtime group roots should inherit the vanilla faction base group prefab, not generic `Prefabs/AI/Groups/Group_Base.et`. The vanilla base groups carry `SCR_EditableGroupUIInfo.m_sFaction` and military-symbol identity used by Game Master; setting only `m_faction` is not enough evidence for the GM card. Still explicitly clear `m_aUnitPrefabSlots`, because faction base prefabs can carry inherited default slots.
- Physical-war infantry spawns should print a root-to-member proof line: zone owner, expected active-group faction, selected prefab, catalog/resource match, runtime status/mode, counts, and visual root/member evidence. This makes a green GM square diagnosable as either an empty root, wrong root UI metadata, wrong selected prefab, or wrong live member prefab/faction.
- Visual evidence for tracked runtime member handles should print member prefab samples before root-shell samples. An empty group root with no native agents is useful context, but it is not proof of the soldier model/equipment the player sees in Game Master.
  - `SCR_RespawnComponent` can be obtainable while `PlayerManager.GetPlayerController(playerId)` still returns null during a connect/setup race. Submitting a native free-spawn request in that window can produce `SCR_FreeSpawnHandlerComponent.AssignEntity_S` VM exceptions on `playerController.GetControlledEntity()`.
  - Gate both setup-holding and active FIA spawn requests on `GetPlayerController(playerId)` before calling `RequestSpawn`; retry on later spawn sweeps instead of treating this as a hard failure.

- Standalone HQ NPC runtime handles need recovery and preparation.
  - A bare character prefab can spawn successfully and still be gone from service tracking by the next HQ lifecycle tick. Before spawning another Petros, scan near the intended HQ/Petros position for an existing matching prefab and reattach the service handle.
  - Petros recovery scans should also accept the prepared runtime identity, not only exact prefab metadata. A prepared Petros is explicitly named `HST_Petros`; accept that identity near the HQ slot, reprepare it, and debounce respawn attempts before creating another character.
  - Immediately after spawn or reattach, set a stable entity name, visible/traceable/active flags, FIA faction, origin, and disabled movement/weapon controls so Petros behaves like a stationary HQ NPC instead of a normal player-controlled spawn candidate.
  - Petros runtime proof must be the real prepared Petros character, not the HQ action surface/tent/cache. Do not make optional `SCR_AIGroup` attachment part of `m_bHQRuntimeObjectsSpawned`; the July 5 dedicated-server run proved grouped Petros could attach successfully, then drop out of service tracking every lifecycle tick while the other HQ pieces stayed usable.
  - If Petros disappears without an HST clear path, log the removal and retry a real character spawn. Do not mark the HQ runtime complete until the character itself is tracked again.

- `AIGroup` prefab spawning is not necessarily populated on the same frame as `SpawnEntityPrefab`.
  - GUID-prefixed prefab paths such as `{...}Prefabs/...` are resource identifiers, not a selector for a specific loaded mod source. Do not diagnose wrong-faction spawning by removing GUID prefixes; prove the selected group prefab, group faction, member slot prefabs, and spawned member prefab/faction instead.
  - In Game Master, a green group square with `Size 0` is an empty group root, not proof that the soldiers themselves spawned as FIA. `SCR_EditableGroupComponent` reports the leader faction when a leader exists; with no leader it falls back to the root `SCR_AIGroup` faction.
  - Game Master group size comes from `SCR_EditableGroupComponent.GetSize()`, which returns `SCR_AIGroup.GetPlayerAndAgentCount()`. Active-group proof should log both native raw/living counts and the editable size/faction; if raw/living members exist but editable size stays zero, the issue is editor visibility/replication, not the primary member spawn.
  - Stock `ConflictMilitaryBase.et` includes inherited `AmbientPatrolSpawnpoint_FIA` children. Do not use it directly as a h-istasi marker-only surface for capturable zones or hideouts. Use the stripped HST conflict marker prefab instead, so the source entity has no ambient patrol component path.
  - Layer-level disables of inherited ambient patrol children are fragile and should be treated as a temporary diagnostic workaround only. Static validation should fail if HST marker or hideout layers reference the stock conflict base, the FIA ambient patrol prefab, or the old inherited ambient-patrol child IDs.
  - AI-heavy reference scenarios place US/USSR soldiers as explicit `SCR_AIGroup` prefab instances and use ambient patrol spawnpoints as separate systems. A Game Master card named like a light infantry squad with `Faction FIA` and `Size 0` near a capturable zone is therefore most consistent with an ambient-patrol shell or empty editor group, not with a GUID-qualified HST stock group prefab resolving to the wrong faction.
  - `SCR_AIGroup.SpawnUnits()` always queues runtime member creation through the group's delayed-spawn list and frame update path; `SetMemberSpawnDelay(0)` does not mean same-frame agents. Game Master can therefore show a root with Size 0 while navmesh tiles, AIWorld budget, or the delayed queue are still pending.
  - `SCR_AIGroup.SpawnAllImmediately()` is a native primary-path queue drain for an existing stock group root, not a direct-infantry fallback. Campaign-debug pre-route probes may use it to make the currently queued `m_aUnitPrefabSlots` members countable before route assertions, but must still report the action as native/stock proof and must not use direct faction infantry as certification success.
  - Native/editor logs can report that a group prefab was spawned while its agents are still invisible or pending creation.
  - Group-owned HQ NPC spawns can legitimately resolve one runtime tick after the command that queued `SpawnUnits()`. Debug cases must not assert `m_bHQRuntimeObjectsSpawned` in the same frame as a rebuild request; wait for the runner's next tick, call `EnsureRuntimeObjects()` again, then prove the HQ state with tracked handles plus world scans.
  - h-istasi mission/physical-war spawning should keep the existing population grace/polling path before declaring a group failed. The July 2026 logs showed HST-spawned groups reporting zero agents first and then later folding/populating correctly.
  - `SCR_AIGroup.GetOnAllDelayedEntitySpawned()` is the native signal that the delayed member-spawn list drained. Subscribe to it for pending groups and use the event to retry the live-agent count early, while keeping a timed retry fallback because navmesh/AI budget failures can still drain the list without immediately yielding countable agents.
  - On a dedicated server, the delayed-spawn event can drain with zero agents and no useful engine warning. Do not immediately replace that group with direct faction infantry; the July 5 runtime run proved direct fallback can print one populated proof line, then leave no durable crew for convoy seating/waypoints on the next tick.
  - Use the vanilla ambient-patrol pattern as the primary stock-group path: spawn the faction-authored `SCR_AIGroup` prefab normally, set delete-empty/max-units/member-delay, then call `SpawnUnits()` only when `GetSpawnImmediately()` is false. Do not force a group-faction broadcast before native member evidence exists; first prove the prefab root's native `m_faction` matches the expected side, then audit or repair delayed members after they exist.
  - Runtime faction repair should not run on an empty/pending stock group root. `SCR_AIGroup.SetFaction()` broadcasts to the editor UI and should be reserved for groups with countable native members or direct fallback members. For the primary path, validate and log the authored root faction before population, then run repair/broadcast only from the populated finalization path.
  - A spawned `SCR_AIGroup` root with `spawn_pending_agents` is not active infantry yet and is not automatically a failed garrison activation. Log it as pending native population with pending infantry/group counts, then let the native delayed-spawn event, bounded retry, stock member-slot path, or final zero-agent failure produce the proof. Only log `activation partial` after no pending native infantry remains.
  - Support-created active groups need their semantic spawn mode preserved while the primary spawn path adds evidence tokens such as `group`, `group_spawn_retry`, or `group_native_immediate`. If `petros_attack_support` is overwritten with plain `group`, static-route normalization can treat it as a normal guard group and collapse the HQ/Petros target back to the spawn position.
  - Post-case and final campaign-debug cleanup must certify that no non-terminal infantry active group remains in `spawn_pending_agents` or `spawn_deferred_aiworld_budget`. Empty Game Master group roots must produce a named `runtime_group_population_settled` blocker with group id, expected faction, prefab, status/mode, live member count, reason, and visual evidence instead of being treated as passable cleanup.
  - `SCR_AIGroup.SpawnUnits()` is gated by `AIWorld.CanLimitedAIBeAdded()`. The vanilla AIWorld prefabs use a 256 active-AI limit, and a loaded scenario can already exceed that before HST activates a zone. Before native active-group population, raise/report an HST physical-war AIWorld limit floor and prove the limited/active counters in the spawn log.
  - ConflictPVE-style ambient patrol code follows the same broad budget discipline: compare `AIWorld.GetCurrentNumOfActiveAIs()` against `AIWorld.GetLimitOfActiveAIs()` before activating a spawn point, pause when over the configured threshold, then spawn the group root and call `SetMaxUnitsToSpawn()`/`SpawnUnits()` when the root is not spawn-immediate. This reinforces treating AIWorld headroom as a primary spawn precondition, not an after-the-fact fallback trigger.
  - If AIWorld still cannot reserve room for the requested native members, do not leave the newly spawned `SCR_AIGroup` root visible in Game Master. Clear the pending callback, delete the empty runtime root, reset `m_bSpawnAttempted`, and record `spawn_deferred_aiworld_budget` so a later service tick can retry without presenting an empty FIA-looking group as spawned.
  - If `SpawnUnits()` drains with zero living controlled members and the native delayed queue is no longer active, the next primary proof path is to spawn the selected stock group prefab's own `m_aUnitPrefabSlots` character prefabs and attach those members to the same `SCR_AIGroup`. This differs from direct faction-infantry fallback because the visible soldier prefabs still come from the selected US/USSR/FIA group prefab.
  - Every zero-agent active-group log should include stock slot count, native queue size, raw/living agent count, `membersToSpawn`, and AIWorld limited/active budget counters. A fallback without these diagnostics hides the primary-method failure.
  - Before direct fallback, call `SCR_AIGroup.SpawnUnits()` once on the original pending group with `SetDeleteWhenEmpty(false)`, `SetMaxUnitsToSpawn(expectedInfantry)`, and zero member delay, then keep the normal retry window alive. This preserves the native prefab/member-slot path that convoy routes and combat waypoints need.
  - Disable empty-group deletion immediately after spawning a native active-group prefab and again when registering the pending-population callback. The July 5 dedicated-server convoy run showed `SCR_AIGroup` roots disappearing before the retry/fallback window, which leaves the logs stuck at `missing runtime group entity` and prevents convoy crews from ever seating.
  - Because HST sets active `SCR_AIGroup` roots to non-deleting while delayed population is pending, terminal cleanup must explicitly unregister/delete the empty group root once the active group is eliminated or failed. Do not use broad entity deletion for this combat cleanup: preserve dead character entities/corpses, remove only the group root/runtime handles, and let the campaign state keep the terminal row for rewards, objectives, and reports.
  - Persistence smoke convoy rows are state-only restart sentinels, not live convoy crew proof. They must not trigger physical convoy crew repair or direct-infantry fallback during normal runtime updates; refresh their spawn mode to a non-fallback data marker on seed so stale fallback metadata from an older save cannot poison primary-spawn certification.
  - If the native group root is already gone by the direct fallback attempt, create the direct faction-infantry replacement group anyway instead of skipping fallback. This keeps the final retry path able to prove a durable faction-correct `AIGroup` or fail with spawned-member evidence, rather than failing only because the original root deleted itself.
  - Direct faction-infantry fallback should spawn the HST-owned faction-specific empty group prefab, not raw `Group_Base.et`, for the same empty-root deletion reason. Use `HST_RuntimeEmptyGroup_US.et` for US, `HST_RuntimeEmptyGroup_USSR.et` for USSR, and only use the FIA root for FIA. The fallback still stamps the requested faction after spawning, so the prefab default is only a bootstrap value.
  - Captive-follow helper groups should use the same `HST_RuntimeEmptyGroup.et` non-deleting root before attaching the captive and stamping CIV. A raw empty `Group_Base.et` can disappear before `AddAgentFromControlledEntity()` verifies a durable follow group.
  - Finalizing active-group population should require living controlled entities from `AIGroup.GetAgents()`, not just `GetAgentsCount()` or tracked direct member handles. Convoys, QRF routes, and physical combat probes all need durable `AIGroup` agents because `AIGroup.AddWaypoint()` and convoy seating operate on the group root.
  - Faction/live-count audits must collect unique living members from native `AIGroup.GetAgents()` plus separately registered direct fallback member entities. An empty native shell must not hide direct fallback proof, but direct-only proof still does not satisfy route, waypoint, or convoy-seat stages that explicitly require durable group agents.
  - Direct faction-infantry fallback is a final retry fallback, not the normal delayed-spawn response. If it cannot produce durable `AIGroup.GetAgents()` members before the retry budget expires, leave the group as `spawn_failed` with the spawn reason instead of marking it populated and letting survivor updates immediately classify it as eliminated.
  - A native delayed-spawn queue can remain `IsInitializing()`/queued without yielding live agents. After the stock group and stock member-slot paths have had their bounded retry window, the final direct faction-infantry repair must be allowed to replace that stuck empty root instead of waiting forever on `IsNativeGroupDelayedPopulationActive()`. Record this as `directFallback` evidence so certification still treats it as degraded behavior, not primary success.
  - Group/entity faction metadata must be audited explicitly after runtime spawn. Native group creation should already read the selected prefab's authored `m_faction`; if the root `SCR_AIGroup.GetFactionName()` differs before `SpawnUnits()`, hard-fail the spawn instead of trying to turn a wrong-faction group into another side. Verify/apply `FactionAffiliationComponent.SetAffiliatedFactionByKey(factionKey)` on controlled agents and direct fallback members after they exist, while active and convoy vehicle entities must have their engine vehicle-faction claim cleared.
  - Controlled stock group spawn should hard-fail if the root `SCR_AIGroup.GetFactionName()` differs from the active group's expected faction before native member spawning. Keeping a wrong-faction empty root alive hides the primary failure and produces misleading Game Master evidence such as a zero-size FIA group at a US/USSR zone.
  - Keep a runtime faction reconciliation pass for active groups. If later delayed agents, direct fallback members, or vehicle entities appear after initial spawn, restamp them before survivor counting and log persistent faction mismatches with expected faction, group id, source, mismatch count, and sample position.
  - Faction proof for infantry groups is not valid with only an `SCR_AIGroup` root. Runtime debug should require at least one live controlled member after the population grace; repeated `members changed 0/0` evidence means the group shell was stamped but the spawned agents were never proven.
  - Base `AIGroup` is useful for generic agent enumeration, but faction diagnostics that call `GetFactionName()` must cast to `SCR_AIGroup`. Workbench reports `Undefined function 'AIGroup.GetFactionName'` when a helper stores the root as the base type and then asks for faction metadata.
  - Campaign-debug faction cleanup audits must not create direct fallback infantry while measuring runtime faction proof. If a spawned group shell still has no live controlled members during or after the bounded population grace, count it as unproven runtime faction coverage rather than repairing the condition inside the audit.
  - Direct faction-infantry fallback should be visibly tagged as degraded runtime behavior. Post-case and final certification should fail any active-group row whose spawn mode contains `direct_infantry_fallback`, including terminal rows, because any fallback row means the primary stock group/member-slot path failed earlier.
  - Runtime faction spawning should route group, direct-infantry, and vehicle selection through a single spawn-spec resolver before calling native spawn APIs. Keeping the requested faction key, expected native faction key, role, and catalog source together makes later runtime faction proof explain which catalog path produced each group or vehicle.
- Build provenance must be single-source and visible in both boot logs and structured debug output.
  - `HST_BuildInfo` owns the structured build sha/utc/label used in campaign-debug JSON and summaries, while the coordinator's authority boot/admin string is what a server operator sees first in logs. Keep them synchronized every time runtime proof behavior changes; mismatched build strings make post-test logs impossible to trust.
  - Campaign-debug physical probes should assert the reconciliation result, not only log it. Convoy and physical-combat cases should fail when checkable crew/group/vehicle runtime entities still expose a faction different from `HST_ActiveGroupState.m_sFactionKey`; pending/uncheckable entities can remain WARN while the spawn grace owns them.
  - When direct fallback members are tracked beside their parent `AIGroup`, counting must not double-count or mask a broken native group. Count unique living native-agent members and direct tracked members for faction proof, and keep separate assertions for systems that require the parent `AIGroup` itself to own durable agents.
  - Prefer faction infantry character prefabs from `HST_DefaultCatalog.CreateFactionTemplate(factionKey)` for fallback members. Do not reuse group prefabs or generic FIA defaults for all sides, or server runs can appear to invert/flatten faction composition compared with Workbench expectations.
  - Character liveness checks should read `CharacterControllerComponent.GetLifeState()` first and only use damage-manager state as a fallback for non-character entities. Newly spawned or fallback AI characters can report usable character life state before damage state is meaningful for convoy/physical-war proof.
  - Game Master/editor placement uses the editor path, not the HST physical-war service path. If editor-placed squads only appear after camera movement, inspect streaming/editor placement addons or native editor activation first; do not assume the HST active-group population guard is involved unless HST spawn logs appear around the placement.
  - Runtime AI contact probes need a real elapsed-time window. Spawn opposing groups, assign normal `AIGroup.AddWaypoint()` search/destroy or move waypoints, then let the standard service tick sample live counts and distance over time instead of trying to prove combat inside the same script call. Treat waypoint assignment/contact distance as setup/contact evidence and live-count loss as separate casualty-resolution evidence.
  - Physical-combat certification must prove native faction hostility before judging combat behavior. Use `Faction.IsFactionEnemy()` in both directions for the temporary resistance/enemy factions, record that relationship as an assertion, and keep contact distance as the hard runtime proof. Treat no live-count loss after proven hostile contact as WARN because casualty timing is stochastic across short server windows.

- Garage vehicles are part of the undercover vehicle-cover runtime truth.
  - When a tracked runtime vehicle is captured into the virtual garage, copy reported state, vehicle heat, cover eligibility, report expiry, last report reason/zone, and passenger compromise count onto `HST_GarageVehicleState` before deleting the world entity. Otherwise a reported vehicle becomes clean by storage rather than by the heat rules.
  - When a garage vehicle is redeployed, copy the same metadata back onto the spawned `HST_RuntimeVehicleState` after prefab/source fields are set. Runtime undercover checks should see the redeployed vehicle exactly as hot or clear as the stored record described it.
  - Full Campaign Debug should prove both handoff directions through the real store/redeploy/capture command paths and include a save-data roundtrip for the captured garage record.

- Entity-follow behavior needs an entity waypoint, not a static move waypoint.
  - The base-game pattern is `SCR_FollowGroupCommand`: spawn `{A0509D3C4DD4475E}Prefabs/AI/Waypoints/AIWaypoint_Follow.et`, cast it to `SCR_EntityWaypoint`, call `SetEntity(target)`, then `AIGroup.AddWaypoint`.
  - A patrol hierarchy waypoint such as `{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et` only moves to the sampled position. Keep it as a last-resort static move fallback, but do not expect it to remain bound to a moving player or vehicle.
  - Do not assume `SetPriorityLevel()` is available on a generated `AIWaypoint`. If a waypoint path compiles as base `AIWaypoint`, omit the priority call or keep it behind a compile-proven `SCR_AIWaypoint` path. Workbench reported HST captive follow calls as `Undefined function 'AIWaypoint.SetPriorityLevel'`; adding the waypoint plus movement type/formation was the compatible path.
  - After adding a follow waypoint, apply `AIGroupMovementComponent.SetFormationDisplacement(1)` so the follower stays close to the target instead of regrouping at a loose formation offset.
  - A direct `AIBaseMovementComponent.RequestFollowPathOfEntity()` or entity-follow waypoint can accept without producing movement. Track owner progress while the target is outside the close-follow distance; after repeated no-progress ticks, skip direct follow for that update and force a refreshed static waypoint at the responsive follow position. Keep the entity-follow waypoint as the preferred path when it is making progress.

## Native Reference Sources

- Native map config reference: `Configs/Map/MapFullscreen.conf`.
- Native map layout reference: `UI/layouts/Map/MapMenu.layout`.
