# h-istasi Enfusion / Enforce Notes

Purpose: capture reusable facts learned while building h-istasi so we do not rediscover the same Enfusion and Enforce Script edge cases repeatedly.

This file is for practical engine/script behavior, not project planning. Keep entries concrete: what failed, why it failed, what works instead, and where the example lives.

## Enforce Expressions And Prefab Inspection

- `string.Format()` accepts at most nine substitution arguments.
  - The generated `string` API exposes parameters `%1` through `%9`; a canonical
    manifest string with more fields must be assembled from several format calls
    or explicit concatenation.
  - Current example: `HST_ForcePlanningIntegrityService.BuildManifestHash()` builds the
    immutable canonical input in bounded chunks before calling `string.Hash()`.

- Enforce Script does not accept the C-style conditional `condition ? a : b`
  expression in normal game scripts.
  - Workbench reports `Broken expression (missing ';'?)` at each conditional
    expression. Resolve the value into a typed local with an `if` first.
  - This matters especially inside `string.Format()` evidence rows, where the
    parser error otherwise points at the whole format call.

- Exact group-roster validation can inspect prefab containers without spawning.
  - Load the group `Resource`, require both a non-null handle and `IsValid()`,
    resolve its `IEntitySource` with `SCR_BaseContainerTools.FindEntitySource()`,
    and prove its class inherits `SCR_AIGroup`.
  - `IEntitySource.Get("m_aUnitPrefabSlots", array<ResourceName>)` returns the
    effective authored slot list in order and preserves duplicates. Validate
    every member resource the same way and require `SCR_ChimeraCharacter` only
    for infantry-only catalog pools.
  - Do not use `SCR_AIGroupClass.GetMembers()` as a static catalog check. It can
    return zero when the game, AI world, or formation dependency is unavailable,
    which is ambiguous with a genuinely empty roster.
  - Current example: `HST_ForceCatalogService.TryReadGroupSlots()` compares each
    execution prefab against the explicit versioned force catalog.

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

- Global player-facing notifications should fan out through each connected player's owner request bridge.
  - A single server-side/player-owned component is not a reliable broadcast origin for UI toasts and mission events in multiplayer, because replication visibility and owner routing can vary by component owner.
  - On the server, enumerate `PlayerManager.GetPlayers()`, resolve each player's `HST_CommandMenuRequestComponent` from their `PlayerController`, and call an owner-targeted RPC for mission events, mission intel, and global notification payloads.
  - Keep `RplRcver.Broadcast` fallback only as a degraded path when no owner bridges are available.
  - Current example: `HST_CommandMenuRequestComponent.BroadcastMissionEventToConnectedOwners()`, `BroadcastNotificationToConnectedOwners()`, and `BroadcastMissionIntelToConnectedOwners()`.

## Player Stamina And Screen Effects

- Player stamina writes must run on the owning client.
  - `CharacterStaminaComponent.GetStamina()` is readable from script, but `AddStamina()` must run on the owner. Server-only refill logic is unreliable on a dedicated server.
  - Use the player-owned request bridge to receive the authoritative `features.infiniteStaminaEnabled` setting and refill the local controlled entity's `CharacterStaminaComponent`.
  - The dark sprint overlay is `SCR_StaminaBlurEffect`, which drives `SuppressionVignette` and radial blur from the character `Exhaustion` signal. Suppress that effect with a modded class while infinite stamina is enabled instead of disabling unrelated damage/bleeding/drowning/poison/optic vignettes.
  - Current examples: `HST_CommandMenuRequestComponent.TickInfiniteStamina()` and `HST_StaminaBlurEffectPatch.c`.

## AI Groups And Game Master

- Editor-mode repair must not change player roles from inside the player-role
  callback that initiated the repair.
  - `SCR_BaseGameMode.OnPlayerRoleChange()` invokes its listeners synchronously.
    The editor listener can repair role-provided modes, whose `AddMode()` calls
    `SCR_EditorManagerEntity.UpdateLimited()`. The base limited-state update can
    then grant `EPlayerRole.GAME_MASTER`, re-entering the same `ScriptInvoker`
    before its first invocation has returned. Enfusion reports this as
    `ScriptInvoker: Recursive call of Invoke!`.
  - Defer only the editor subsystem's role-change listener through the game call
    queue. Its next-frame mode repair can use the stock limited-state update
    after the original invoker has returned. Do not defer every `UpdateLimited()`
    call: editor-manager teardown removes modes through that path and must not
    leave callbacks queued against a deleting entity. Let the stock methods keep
    ownership of limited-state calculation and role grant/clear behavior.
  - A recoverable VM exception can still create `crash.log` while the dedicated
    server continues running, so distinguish the exception timestamp from the
    later intentional disconnect/shutdown before classifying the whole run.
  - Current example: `HST_EditorRoleChangeReentryGuard.c`.

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

- Tracked runtime members can be the only reliable repair source when Game Master shows a live group as Size 0.
  - Dedicated-server population can leave HST with valid controlled member handles while the native `SCR_AIGroup` still reports no agents or the member editable components are not parented under the group editable component.
  - Reconcile from HST's tracked runtime member arrays as a fallback: re-add each living member's `AIAgent` or controlled entity to the `SCR_AIGroup`, activate the group/agent pair, and reparent member editable components under the group root. Do not treat a Size 0 card as terminal while tracked live members still exist.
  - Current examples: `HST_PhysicalWarService.ReconcileTrackedRuntimeMembersWithAIGroup()`, `ReconcileRuntimeGroupEditableMembership()`, and `AttachFactionInfantryMemberToRuntimeGroup()`.

- Group membership repair must not claim nearby same-faction soldiers by proximity.
  - A broad world scan around a group root can sweep unrelated garrison, support, civilian-security, or mission soldiers into the wrong group, producing misleading member counts and repeated reconciliation spam. Use native group agents and explicit HST tracked member handles as proof; if those are missing, treat the group as unproven instead of attaching nearby entities by distance.
  - Repeated editable/group-count diagnostics should be throttled. A dedicated server can hit the same active group every runtime tick, and per-tick `active group` logging can become a performance problem before it helps the count issue.
  - Current examples: `HST_PhysicalWarService.DiscoverRuntimeGroupMemberHandles()`, `ReconcileTrackedRuntimeMembersWithAIGroup()`, `ReconcileRuntimeGroupEditableMembership()`, and `DebugLogThrottled()`.

- Response infantry speed should be enforced at the group and agent level.
  - `AIGroupMovementComponent.SetGroupCharactersWantedMovementType(EMovementType.RUN)` is the correct group-level control, but delayed-populated members can still appear after the first waypoint assignment.
  - Reapply response speed during routed updates, set tight formation displacement, and also set each live `AICharacterMovementComponent.SetMovementTypeWanted(EMovementType.RUN)` so support/QRF/Petros attack forces do not inherit a walking default.
  - Current examples: `HST_PhysicalWarService.ApplyResponseGroupMovementSpeed()` and `CampaignDebugIsActiveGroupResponseRunMovement()`.

- Zone garrisons need patrol routes after runtime population.
  - Static garrison groups can spawn with valid agents but no useful route assignment, which makes the group look alive in Game Master while every unit stands idle. After the group has a runtime root and living agents, route it through the same cyclic waypoint helper path used by town police unless it is a convoy, roadblock, mission-owned group, town-security projection, or terminal support group.
  - Current example: `HST_PhysicalWarService.UpdateZoneGarrisonPatrols()`.

- Non-deleting `SCR_AIGroup` roots need explicit terminal cleanup.
  - HST keeps active group roots from auto-deleting while delayed native member population is pending. Once every controlled member is dead, the group root should be unregistered/deleted so Game Master does not retain an active Size 0 group icon.
  - `SCR_EntityHelper.DeleteEntityAndChildren()` deletes the entity tree. If dead controlled members were editable-parented below the group root, detach their editable parent before deleting the group root so bodies and loot can remain while the stale group icon disappears.
  - Current examples: `HST_PhysicalWarService.DetachDeadRuntimeMembersFromGroupRoot()`, `CleanupTerminalActiveGroupRuntimeCrew()`, and `UnregisterRuntimeCrewHandlesForRespawn()`.

- Persistence smoke convoy rows are state-only restart sentinels.
  - They should not trigger physical convoy crew repair, stock-slot population, or direct fallback, because that contaminates primary-spawn certification with fake smoke groups.
  - Seed them with a non-fallback data-only mode and skip physical repair for smoke mission/group ids.
  - Current examples: `HST_PersistenceSmokeTestService.EnsureSmokeActiveGroup()` and `HST_PhysicalWarService.CanAttemptMissionConvoyCrewPopulationRepair()`.

## Runtime Architecture Patterns

- Authored strategic structures need a durable site owner before a mission owns
  an asset.
  - Freeze one canonical site/target identity, prefab, and position after an
    unambiguous world query. Treat the entity handle as disposable projection
    evidence; handle loss is neither destruction nor permission to spawn a
    fallback. Keep that stable target ID separate from the unique physical
    runtime-entity ID of each mission projection.
  - Distinguish BORROWED_WORLD from GENERATED_CAMPAIGN ownership. Ordinary
    mission/composition cleanup may forget a borrowed entity but must never
    delete it. Only the site owner may delete its generated projection.
  - Query ambiguity must be checked before changing health or deleting anything.
    Descriptor children and light/base transmitter variants can otherwise make
    a broad query appear to identify multiple towers. Normalize to the root,
    prefer concrete supported prefab identity, collapse duplicate handles, and
    quarantine cross-site claims instead of choosing by iteration order.
  - A destroyed borrowed structure can reappear intact when its base-world layer
    streams or a process restarts. Rebind near the frozen position and reapply
    the destroyed damage state. For a rebuilt/generated replacement, suppress
    the resurrected authored original before projecting the generated target.
  - A rebuild-interdiction objective should project construction equipment, not
    another intact copy of the strategic structure. Keep the canonical future
    transmitter binding on the site while the mission asset uses its own typed
    equipment prefab. Allow only one interdiction attempt for each underlying
    structure-destruction receipt; destroying the equipment must not create a
    fresh structure-destruction epoch.
  - Current examples: `HST_RadioSiteLifecycleService`,
    `HST_RadioSiteSaveValidationService`, and
    `HST_RadioRebuildEquipment.et`.

- Owner-bridge command RPCs need caller-generated request identity.
  - Carry the same request ID from the player-owned request component through the server RPC and coordinator dispatch. Persist a bounded command receipt for each ID so an exact replay returns the prior result without mutation, while reuse with a different command or payload is rejected as a conflict.
  - Treat the receipt as campaign state, not transient UI state; save/load must preserve idempotency across reconnects and restarts.
  - Once a command has a typed domain result, derive `APPLIED`/`REJECTED` and
    aggregate identity from that result. Never search user-facing prose for a
    word such as `failed`; an accepted terminal outcome can legitimately
    describe the deployment failure it settled. Keep the text classifier only
    as an explicit compatibility path for commands not yet migrated, and mark a
    newly inserted receipt as a persistence change even when the domain result
    itself was unchanged or rejected.

- Paid campaign mutations need a transaction ledger, not inline balance edits.
  - Use explicit reserve, commit, cancel, and refund transitions. Restore reconciliation must cancel/refund any open reservation before normal service ticks resume. Campaign events and command receipts are bounded; schema 48 introduced bounded accepted garrison/QRF quote, manifest, and linked transaction history, which schema 49 retains.
  - Training is the first production consumer; exact garrison confirmation is the second; exact player QRF is the first paid-support consumer of one frozen manifest across quote, charge, spawn, persistence, recall, and refund. Other paid support remains outside this contract.

- A player-visible force purchase needs two server-authoritative commands.
  - The first command validates target/capacity/catalog/resources and persists an immutable, expiring quote plus exact manifest. It does not debit anything. The UI then displays the returned accepted count and cost.
  - Confirmation submits only the quote ID. Revalidate actor, active-campaign/HQ state, expiry, the member catalog/hash, ownership, zone kind, capacity snapshot, and resources; reserve each resource with deterministic transaction IDs; register the exact purchase-time strategic increment and acceptance provenance; verify the exact delta and one acceptance link; then commit every reservation.
  - The accepted quote status is the replay guard while full rows remain. After terminal settlement compaction, the persisted settlement tombstone owns issue/confirmation/transaction replay. A repeated confirmation must never require recreating the aggregate or debit: later activation, casualties, fold-back, or zone cleanup can legitimately replace the original purchase-time link.
  - Quote confirmation is synchronous, but restore still treats any `ISSUED` quote with linked reservations, commits, or an accepted-garrison link as interrupted. Reconciliation removes the purchase-time aggregate contribution when it can prove the link, cancels/refunds every linked money/HR transaction that exists, and rejects the quote before the generic open-reservation sweep runs.
  - Bound concurrent open quotes independently from historical settlements. Keep accepted rows in full for a minimum age and every live backlink, then insert the compact replay row before removing the transaction, manifest, quote, and growing garrison-provenance link. Full rows plus tombstones need one hard admission bound; replay-window expiry may remove only the oldest tombstone and an evicted ID must fail closed.
  - `ResourceLedgerService.ReserveCost()` must consult archived transaction IDs before `SpendResource()`. Exact replay of an archived committed transaction may return an ephemeral already-applied row; identity conflict or a refunded/cancelled row must not debit or reopen the transaction.
  - A tested queue compaction API is not production maintenance. Build pins from nonterminal support/orders and every retained active group, call `CompactTerminalRows()` from normal and terminal coordinator ticks, and return its state change through persistence tracking. Otherwise the 128-row terminal cap becomes a permanent admission deadlock and settled manifests never become archive-eligible.
  - Legacy aggregate/admin mutation helpers must remain visibly separate from the exact paid path. Both visible-command dispatchers route `recruit_zone` only to quote issuance; caller-priced coordinator wrappers are protected/internal. Do not manufacture historical member slots, prices, transactions, or refunds during migration.
  - Current examples: `HST_ForcePlanningService.IssueGarrisonQuote()`, `ConfirmGarrisonQuote()`, `HST_GarrisonService.AddManifestForcesExact()`, and schema-43 migration normalization.

- Full Campaign Debug must fail closed outside an isolated development session.
  - It can force terminal outcomes, mutate resources and campaign rows, and create world/player side effects. Persist the live campaign first, retain its original `HST_CampaignState` reference, run the sequencer against a deep save-data clone, divert persistence `Tick`, `MarkMajorChange`, checkpoint, capture, profile-fallback, and save-point paths, then swap the untouched reference back on both completion and cancellation. Applying a save snapshot into the mutated object is weaker because transient state can be omitted.
  - External/restart/soak profiles require a separately managed disposable profile and launcher; do not let their labels enter the common in-process bootstrap/HQ/checkpoint steps.
  - Campaign-state swapping does not restore actor position, health, inventory, world entities, AI groups, vehicles, waypoints, service caches, or delayed callbacks. Keep the exact development-world gate and require a session restart before treating the runtime as clean.

- Campaign marker publication is not proof that its visual widget is ready.
  - A published model or native/static marker handle can exist while the delayed map widget root is still null. Visual proof must open the map, wait through widget construction, inspect static campaign-marker root readiness, and surface any update-time VM exception; model and handle counts alone are insufficient.
  - The stock manager can enable its update loop after `OnCreateMarker()` returned early for a missing map root/frame/config/layout. Guard the manager before it calls `SCR_MapMarkerBase.OnUpdate()`: retry `OnCreateMarker(true)`, and move a still-rootless marker to the disabled list so the normal map-pan/open recovery path can retry it safely.
  - Census active and disabled static arrays separately. Every active static marker needs a root and `SCR_MapMarkerWidgetComponent`, and at least one active root should be visible when the map is open; disabled-rootless markers are diagnostic because off-frame markers may legitimately be disabled. Include only bounded marker ID/type/config/owner samples.
  - Server publication and reconciler handle counts must be labeled as handles, never widgets. Widget readiness is owner-client render evidence only.

- Headless Workbench script validation needs the project and Script Editor run mode explicitly.
  - Use the generic parameter shape `-gproj <project> -wbModule=ScriptEditor -run -validate PC -wbsilent -exitAfterInit`; an invocation that only initializes Workbench is not compile/validation evidence.
  - The validation log must name the intended addon project and reach the
    `Module: Game` compile plus successful game creation. A missing-project or
    fallback validation can report success without loading the addon's complete
    class surface and is not evidence for opening the mod.
  - The schema-50 validation run demonstrates why the completion boundary matters.
    The first correctly quoted launches reached Game script compilation but
    terminated with native `0xc0000374` before `Module: Game`. Isolation proved
    that the already-large Phase-20 civilian population debug method crossed a
    native compiler edge when five appearance/horn count locals were added;
    production civilian/strategic services and the new projection proof compiled
    independently. Split the post-selection probe and aggregate related counts
    in one result object instead of shaving assertions. After that refactor,
    headless script validation exits successfully with 5,747 Game files/11,508
    classes and CRC `16665f19`, and a normal WorldEditor open remains responsive
    for all ten samples over 20 seconds. These are compile/startup gates, not a
    packaged-runtime certificate.

- Player-facing economy reports should consume the same service-owned income math as the tick path.
  - If the Command Menu repeats income formulas locally, report totals can drift from actual money/HR mutation as town support, resource types, factories, seaports, airfields, or bank effects change.
  - Keep category/source breakdowns in the town service, then have member inspection append that report so next-income totals, source totals, and per-zone rows are all derived from `CalculateResistanceIncome()`, `CalculateResistanceHRIncome()`, `CalculateZoneMoneyIncome()`, and `ResolveZoneHRIncome()`.
  - Civilian population does not need a separate initial-population durable field for income scaling. Derive the surviving share from `populationRemaining / (populationRemaining + populationKilled)` so old saves and influence events keep working without migration, and let missing civilian records fall back to full income.
  - Current examples:
    `HST_TownService.ResolveTownPopulationIncomePercent()`,
    `HST_TownService.BuildIncomeSourceBreakdown()`,
    `HST_TownService.BuildIncomeReport()`, and
    `HST_CampaignCoordinatorComponent.RequestMemberInspectEconomy()`.

- Training caps should be resolved from campaign state, not saved as their own durable field.
  - Keep the rule in `HST_RecruitmentService.ResolveTrainingCap()` so command actions, reports, and debug probes cannot drift. The current cap is war level plus two, clamped to level 10.
  - Capped training should block without spending money or lowering existing training if a save already has training above the current cap.
  - Training quality is also derived, not saved. `HST_RecruitmentService.ResolveTrainingQualityBonusPercentForLevel()` adds 5 percent effective infantry strength per level above 1, capped at +45 percent. Use `ResolveTrainingEffectiveInfantryStrengthForLevel()` for abstract pressure instead of duplicating the formula.
  - Recruitment reports should show `training current/cap` plus quality, force composition should report effective manpower for resistance forces, and Full Campaign Debug should prove low-war cap blocking, higher-war advancement, and high-training capture-strength effects using isolated fixture states.
  - Current examples:
    `HST_RecruitmentService.TrainTroopsDetailed()`,
    `HST_RecruitmentService.BuildRecruitmentReport()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugTrainingWarLevelCapCase()`.
  - Current quality examples:
    `HST_ForceCompositionService.ResolveEffectiveManpower()`,
    `HST_ZoneCaptureService.BuildCaptureStatus()`,
    `HST_PhysicalWarService.ApplyTrainingQualitySummaryToActiveGroup()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugTrainingQualityCase()`.

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

- Civilian runtime behavior needs owned helper cleanup.
  - Spawned town pedestrians should be attached to a CIV `SCR_AIGroup` and given real cyclic waypoint helpers. The civilian runtime owns those group/waypoint helpers by the spawned character so town cleanup deletes the helpers with the pedestrian.
  - A dynamically spawned civilian group must inherit `Prefabs/AI/Groups/Group_Base.et`. A bare `SCR_AIGroup` resource has none of the stock behavior-tree, formation, pathfinding, movement, utility, editable, or replication components required by the waypoint and multiplayer paths.
  - Initial ambient AI composition should use `SCR_AIGroup.AddAIEntityToGroup()` with a direct `AIGroup.AddAgent()` fallback. `AddAgentFromControlledEntity()` also broadcasts the player-group member-state RPC; it is the wrong initial-attachment path for a newly spawned ambient group and produced one unregistered-item replication error per pedestrian or traffic driver when the group was bare.
  - Ambient civilian traffic uses runtime kind `CIV_TRAFFIC_VEHICLE`: spawn an unclaimed civilian vehicle, spawn a CIV driver helper, register the vehicle with the driver's AI group utility component, seat the driver in the pilot slot, and assign cyclic road-biased waypoints. For server-owned drivers, prefer forced authority-local `GetInVehicle()` and keep `MoveInVehicle()` as the fallback when direct local entry is unavailable/rejected or the entity is non-local; dispatch acceptance alone is not occupancy proof. If registration, seating, or route-helper creation fails, delete the failed vehicle plus owned helpers and remove its runtime row so the inert projection does not consume the town's traffic target indefinitely.
  - Traffic cars are disposable ambience. If a traffic vehicle leaves the configured player render bubble, delete the vehicle and every driver/group/waypoint helper owned by it instead of keeping an abstract traffic record alive.
  - Civilian character prefabs can arrive with an existing parent AI group or inherited faction affiliation. Force both the spawned member and any existing/new civilian AI group to `CIV`, and audit the actual `FactionAffiliationComponent` rather than trusting only HST runtime kind bookkeeping.
  - Full Campaign Debug Phase 20 should assert not just spawned counts, but pedestrian helper coverage, traffic vehicle count, traffic helper coverage, actual CIV faction-component matches, and cleanup of all runtime entities.
  - Helper cardinality proves only that group and waypoint entities were created. Keep distance-over-time pedestrian/traffic movement and zero unregistered-group RPCs as separate runtime acceptance gates; a componentless group can satisfy helper counts while remaining unable to move.
  - Current examples:
    `HST_CivilianService.AssignCivilianPedestrianBehavior()`,
    `AssignCivilianTrafficBehavior()`,
    `ResolveRuntimeEntityFactionKey()`,
    `ApplyCivilianAIGroupFaction()`,
    `CountRuntimeEntityFactionMismatchesForZone()`, and
    `PruneAmbientTrafficOutsideRenderBubble()`.

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

- Enemy town-police groups should be size-specific stock `SCR_AIGroup` prefabs.
  - Keep town-police selection in the catalog instead of scattering prefab ids
    through the physical-war and composition services. `HST_DefaultCatalog`
    owns the size 2/3/4/5 mapping, while force composition and active town
    activation both resolve through that helper.
  - Populate these HST group prefabs with the owning enemy faction's randomized
    soldier character prefab. This keeps the current physical police slice
    faction-correct without maintaining role-specific police loadout lists.
  - Town-police active groups are the exception to generic static-garrison route
    cleanup. Assign their town patrol route through a dedicated resolver, then
    attach `AIWaypointCycle` patrol helpers after runtime agents exist. Register
    those helper entities in the runtime group waypoint arrays so normal group
    cleanup removes them with the spawned police group.
  - Civilian town `policePresence` can create a runtime-only town-police
    projection even when the town has no abstract garrison. Tag these groups
    with `town_security_police`, exclude them from active-garrison and pending
    population checks, and delete them during deactivation/ownership cleanup
    instead of folding them into `HST_GarrisonState`.
  - Current examples:
    `HST_DefaultCatalog.ResolveTownPoliceGroupPrefab()`,
    `HST_ForceCompositionService.TryBuildTownPoliceGroupPlan()`, and
    `HST_PhysicalWarService.UpdateTownPolicePatrols()`.

- Time-limited shop missions should be state-first and delivery-owned.
  - The seller and delivery vehicle are mission assets, not generic cargo or
    capturable loot. Mark the seller as a stationary civilian interaction asset
    and mark the delivery vehicle with a dedicated runtime kind so the garage
    capture path can reject it cleanly.
  - Use the runtime/catalog arsenal item set to build stock instead of keeping a
    hardcoded item list. Persist generated stock and purchased counts on the
    active mission so expiry, save/load, command-menu rows, and delivery can all
    agree on the same reserved purchase state.
  - After a purchase, cap remaining shop time only when it is above the desired
    window. If the shop expires with no purchases, do not spawn delivery or
    apply a penalty; if purchases exist, keep the expired mission alive until
    delivery deposits the items and marks objectives complete.
  - Current examples:
    `HST_MissionRuntimeService.EnsureGunShopStockGenerated()`,
    `HST_CampaignCoordinatorComponent.RequestMemberBuyGunShopItemReport()`,
    and `HST_LootService.CaptureNearbyVehicleToGarage()`.

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
    attack/support resources, HQ knowledge, and vehicle report heat.
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
  - Runtime vehicle reports should use
    `HST_StrategicService.BeginVehicleReportEvent()` before applying the heat
    mutation in `HST_CivilianService.RegisterVehicleHeat()`, then complete the
    event after the vehicle report fields are updated. The strategic row should
    preserve vehicle runtime id, heat before/after/delta, reported before/after,
    and report-expiry delta. Full Campaign Debug should prove both the direct
    report and passenger-exposure report branches plus save-data roundtrip and
    cleanup.
  - Town influence should use
    `HST_StrategicService.BeginTownInfluenceEvent()` before applying
    `HST_CivilianService.RegisterInfluenceEvent()` deltas, then complete the
    event after support and owner fields are updated. Keep these rows compact:
    source id is the town influence event id, target zone is the affected town,
    and the summary/reason carries the influence kind and source context.
  - Radio tower support drift should run from `HST_TownService` on the normal
    income/resource cadence, not from a per-second civilian tick. Pick only the
    nearest eligible owned radio tower per town in the bounded influence radius,
    skip saturated rows that would not change support/reputation/heat, and route
    the result through `HST_CivilianService.RegisterInfluenceEvent()` as
    `radio_broadcast` so the same town influence and strategic-event ledgers
    explain friendly and hostile broadcasts. Full Campaign Debug should keep
    `town_influence.radio.runtime` proving cadence, both drift directions,
    compact strategic rows, and report visibility.
  - Police and roadblock security pressure should also run from
    `HST_TownService` on the income/resource cadence. Derive target pressure from
    town owner relation, wanted heat, occupier-vs-FIA support margin, and war
    level, then move only one police/roadblock step per tick through
    `HST_CivilianService.RegisterInfluenceEvent()` as `security_pressure`.
    Resistance-held towns should shed enemy security pressure unless heat is
    extreme. Full Campaign Debug should keep
    `town_influence.security_pressure.runtime` proving enemy pressure growth,
    resistance relief, compact strategic rows, report visibility, and save-data
    preservation.

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
  - Vehicle heat reports are also strategic-event rows under schema 38, so keep
    the vehicle-cover block, the report output, and the `vehicle_reported`
    strategic event in sync.

- Undercover appearance and weapon eligibility should prefer live equipment
  components, then fall back to identity strings.
  - Equipped clothing can be inspected through
    `SCR_CharacterInventoryStorageComponent` slots; weapon exposure can be
    inspected through `BaseWeaponManagerComponent.GetCurrentWeapon()` and
    `GetWeaponsList()`. Carried inventory should be recursively scanned through
    `SCR_InventoryStorageManagerComponent.GetItems()` plus nested
    `BaseInventoryStorageComponent` slots so weapons inside containers are not
    missed.
  - Keep every block as a reason string (`BLOCK live weapon/equipment`,
    `BLOCK live military clothing/equipment`, or the fallback identity reason)
    so request denial, enforcement scoring, and Full Campaign Debug evidence
    stay explainable.
  - Current examples:
    `HST_CivilianService.ResolveClothingEligibilityReason()`,
    `ResolveWeaponEligibilityReason()`, and
    `HST_CampaignCoordinatorComponent.BuildCampaignDebugUndercoverIdentityGateCase()`.

- Police and roadblock undercover scans should be factorized and explainable.
  - Use a deterministic chance/roll model instead of a hidden random or fixed threshold so server logs, reports, and Full Campaign Debug can reproduce the same outcome. The chance should scale from local security presence, town/player heat, war level, enemy aggression, and blocking eligibility evidence such as a military vehicle or visible weapon.
  - Failure/pass reasons should include chance, roll, war, aggression, presence, town heat, and player heat. This makes a compromise actionable to the player and keeps debug evidence tied to campaign state instead of just saying a scan failed.
  - Current examples: `HST_CivilianService.CalculateRoadblockScanChance()`, `CalculatePoliceScanChance()`, `BuildUndercoverSecurityScanReason()`, and `HST_CampaignCoordinatorComponent.BuildCampaignDebugUndercoverSecurityScanScalingCase()`.

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

- Physical support groups should be state-first and runtime-gated by the player event bubble.
  - Ground support/QRF/search/roadblock groups are not normal zone garrisons, so
    generic zone deactivation intentionally skips them. The support lifecycle
    should create the durable support request and active-group row even when the
    target is off-screen, then keep that group simulated instead of spawning
    runtime entities outside the player event bubble.
  - Simulated support groups can advance their `m_vPosition` through the routed
    active-group loop without live entities. Runtime group/vehicle creation is
    deferred until the simulated group position enters the player event bubble;
    do not mark these rows as spawn-failed or resolved just because they are
    off-screen.
  - Fold-back is still the terminal/recall cleanup path: once a support group is
    intentionally folded, return only the active group's recorded survivor
    infantry and vehicle counts to the abstract garrison, delete live runtime
    handles, and keep the folded group row long enough for support and
    enemy-order synchronization.
  - Current examples: `HST_SupportRequestService.ApplyActiveSupport()`,
    `HST_PhysicalWarService.ShouldDeferActiveGroupRuntimePhysicalization()`,
    `HST_PhysicalWarService.UpdateActiveGroupRoutes()`,
    `HST_PhysicalWarService.FoldActiveSupportGroup()`,
    `HST_CampaignCoordinatorComponent.RunCampaignDebugSupportSimulatedPhysicalizationCase()`,
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

- Routed non-support response groups can consume generated route waypoint chains
  before adding more runtime AI controls. Spawned support/QRF is the explicit
  exception described in the live support route section below.
  - For non-support active groups, resolve the route id against generated routes,
    fall back through the target zone's route ids, then use the generated
    `route_<zone>_alpha` convention before falling back to direct source-target
    movement. Spawned support instead builds a fresh current-live-position to
    target/exit chain; an authored route can be oriented for the wrong leg.
  - Keep generated route positions available while the runtime AI group is still
    pending native population, but persist assigned AI waypoint count only after
    the group is populated. Move live entities only after the group is spawned.
  - Current example: `HST_PhysicalWarService.UpdateActiveGroupRoutes()`.

- Routed infantry AI waypoints need explicit lifecycle ownership.
  - Assign route waypoints only after the active group's runtime `AIGroup` has
    populated. Use generated-route positions for non-support groups and a direct
    current-to-target/exit chain for spawned support, skip the group's current/
    source position, and require at least two assigned waypoint entities before
    marking the active group with `infantry_waypoints`.
  - Use patrol waypoints for intermediate route legs and a search/sweep waypoint
    for an inbound final target; recall exit chains must not end in a sweep.
    Mark the group with `infantry_sweep` only after the final target waypoint is
    actually assigned.
  - Track waypoint entities by active group id and delete them in the same
    cleanup path as runtime crew entities. Otherwise repeated debug runs or
    fold-back cleanup can leave orphan waypoint entities in the world.
  - Keep elapsed-time route interpolation only for eligible unspawned abstract
    groups. Once support infantry is spawned, persist its living-member centroid
    and never teleport the runtime group root or overwrite physical position with
    abstract progress.
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

- Live enemy commander orders need a local front for non-resistance targets.
  - Same-faction defensive targets are valid because they are already the acting faction's holdings.
  - Resistance-held targets are the explicit exception and may be attacked even when they are not near the acting faction's current holdings.
  - Rival or neutral targets should be admitted only when the acting faction has a same-faction foothold within the local-front radius or an explicit linked-zone relationship. Otherwise the conflict should remain background pressure instead of creating visible live supports or capture attempts in disconnected territory.
  - Apply the same locality gate in both target scoring and direct order queueing so debug/admin entry points do not bypass normal commander behavior.
  - Current examples: `HST_EnemyCommanderService.IsLocalOperationTargetAllowed()` and the `enemy_target_scoring.local_front_gate` Full Campaign Debug assertion.

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
  - Schema 44 introduced the durable authority kernel in `HST_ForceSpawnQueueService`; schema 45 connects its first exact engine-facing adapter. Admission requires a frozen, hash-valid manifest whose group, vehicle, member, and asset slots are all required and whose dependencies resolve. An empty group list remains a hard rejection. Schema 54 policy-v2 garrison purchases therefore freeze one executable `NotSpawned` container root plus the arbitrary priced member slots; the root exists as execution authority but contributes no self-populated soldiers. Historical policy-v1 purchase manifests remain nondeployable provenance and must not gain an implicit broad-alpha fallback.
  - Queue identity is per result, request, and projection. A manifest can be projected more than once, so its ID/hash binds immutable input but must not be used as the unique projection or idempotency key. Exact replay is accepted only when the entire frozen payload still matches; key reuse with another payload fails closed.
  - Acquire at most two batches and eight actions per service tick, ordered by cleanup first and then priority/FIFO. Cleanup walks assets, members, vehicles, then group roots so dependency removal remains monotonic across bounded waves. The active bounds are 64 nonterminal batches, 512 nonterminal slots, and 64 slots per request.
  - Schema 45 persists explicit force and projection IDs on the linked active-group row. Missing pre-schema-45 IDs may be derived only from one conflict-free spawn batch; ambiguous links stay empty rather than borrowing identity from a near match.
  - New adapter success must return the same result/projection/slot/attempt generation and prove exact prefab, liveness, faction, native-group membership, Game Master editable-hierarchy membership, projection membership, and any applicable seat evidence before registration. Game Master proof is durable slot evidence, not an inference from native group size. Stale callbacks must not acquire authority over current entities. Retry only failed slots, keep verified siblings, and complete required cleanup before final failure or cancellation becomes terminal.
  - Persist an actual-restore sequence separately from the queue-reconciled sequence. Applying persisted state increments the former once; coordinator initialization reconciles the queue once for that epoch before garrison confirmation and open-ledger reconciliation. Nonterminal work clears process-local evidence and advances its generation. Terminal status/prefab/verification history remains, but entity and native-group IDs are cleared because they are historical observations, not living authority.
  - Terminal compaction requires explicit backlink pins, preserves at least 600 seconds, and opens admission room at the 128-row bound only from eligible oldest rows. Never infer empty pins when callers have not supplied retention ownership.
  - `HST_ForceSpawnAdapterService` is driven once per active-campaign second by the production coordinator. Its first bounded slice supports exactly one infantry `SCR_AIGroup` root plus every frozen member slot. Vehicle, asset, and multi-root manifests fail closed as unsupported rather than silently dropping slots or using broad composition.
  - Slot registration is not projection success. When every exact slot is verified, persist `HST_FORCE_SPAWN_READY_FOR_HANDOFF` as a nonterminal state. First let `HST_PhysicalWarService.FinalizeForceSpawnProjection()` validate and acquire the exact runtime projection; only then call `HST_ForceSpawnQueueService.CompleteProjectionHandoff()` to record `SUCCEEDED`. This two-phase boundary prevents durable success from preceding the authoritative physical handoff.
  - `HST_PhysicalWarService` keeps a narrow queue-ownership bridge and holds its older spawn, repair, survivor, route, patrol, count, and cleanup paths until the exact projection is ready, preventing a queue-owned group from being populated twice. Release ownership only after successful handoff or after failed/cancelled cleanup finishes.
  - This authority queue is distinct from `SCR_AIGroup`'s native delayed member-spawn queue. The former owns durable campaign work across save/restore; the latter is transient engine population state for one live group root. A successful queue row remains historical projection evidence: successful runtime restore/reprojection and a durable casualty/living-force/retirement ledger are still separate ForceRuntime work.
  - Restoring `READY_FOR_HANDOFF` must clear process-local entity/group evidence, advance attempt generation, and requeue exact realization. Never infer that the pre-save physical handoff completed. Terminal `SUCCEEDED` rows remain historical evidence and still need a separate successful runtime restore/reprojection design.
  - Normal acquisition remains active-phase only. Setup and won/lost phases cancel all nonterminal queue work and continue bounded cleanup through a monotonic runtime-only clock. That clock advances retry/defer eligibility without changing the persisted campaign elapsed second, so campaign time stays frozen while dependency cleanup can finish.
  - The exact physical HST_Dev proof service is implemented, including root-before-members, cancellation cleanup, exact registration, handoff, retirement, and same-wave failure cleanup cases. It proves only that a harness path exists until it runs in a fresh isolated world. Do not promote static inspection, deterministic queue assertions, a successful script compile, or project-open survival into physical entity-registration evidence.

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
  - Campaign marker publication and player-tracking marker publication must remain separate. If the optional player marker config entry cannot load, fail that overlay closed for the session and continue publishing HQ, zone, mission, support, and order markers through the campaign marker reconciler.
  - Late join/spawn should force a native campaign-marker republish. Native static marker handles can look published to the server while a newly joined client has not received/constructed the expected map widgets yet; clearing the campaign reconciler and rebuilding from durable marker state is safer than assuming the old handles are visible.
  - Player-requested resistance support has two marker lifetimes: the request marker follows queued/active call-in state, and the live group marker follows the spawned active group. `features.trackResistanceSupportGroupsOnMap` defaults to enabled; when it is on, keep the live marker linked to the active group id until the group is terminal, killed, folded, or despawned. Do not hide the live marker just because the request has reached resolved/arrived status.
  - Keep marker icon deconflicts synchronized in both `HST_MapMarkerService` and `HST_CampaignMapMarkerDirector`. Current campaign semantics: towns use `OBJECTIVE_MARKER2`, military installations use `FORTIFICATION`, radio towers use the injected native `radio-signal` placed-marker icon, roadblocks use `JOIN3`, live resistance support groups use `DOT`, gun-shop seller/delivery markers use `MARK_QUESTION`/`MARK_EXCLAMATION`, destroy missions use `DESTROY2`, rescue POW missions use `HELP`, and generic incoming response support remains on `OBJECTIVE_MARKER`. Stale resolver values can make Workbench previews and runtime native markers disagree.
  - Native placed-marker configs can be patched at runtime when the stock placed marker entry does not expose a needed quad from another imageset. Publish the imageset/quad identity in the desired marker record, resolve or append it against the active placed-marker entry, and validate the resolved entry before creating the native marker. Do not hard-code the appended array index: stock updates can change the inherited icon count. Reapplying an existing entry should also repair its glow resource and category so hot-reloaded legacy entries do not remain half-configured.

- Authored strategic structures need one durable site owner, not a generic
  composition preference.
  - Create one persisted logical site per configured radio zone before world
    binding. Bind only when exactly one supported transmitter root is found in
    range; freeze its prefab and position, mark it borrowed, and never delete
    it. An absent or ambiguous initial target must remain unresolved instead of
    silently spawning a substitute.
  - Entity presence is projection evidence, not lifecycle authority. Persist
    ONLINE/DESTROYED/REBUILDING, deterministic transition requests, receipts,
    and a monotonic revision. A missing handle never proves destruction; observe
    authoritative damage state on the borrowed authored target or the exact
    demolition threshold for generated/equipment projections. Borrowed direct
    destruction requires reciprocal site/mission lock and revision plus the
    tracked authoritative damage state. Generated explosive scoring additionally
    requires a matching live mission component, bounded position, and durable key.
  - Remove the site from generic composition and mission-runtime spawn, repair,
    polling, interaction, and cleanup ownership. Exact and quarantined site
    assets must never fall through to those paths.
  - A stop-rebuild objective should destroy construction equipment, not spawn a
    second intact transmitter. Transfer to generated ownership when rebuild
    begins; create one replacement only when failure/expiry/stop settlement
    completes the rebuild. Suppress a resurrected authored target without
    deleting it, so later stream re-entry cannot create two operational towers.
    Record at most one stop attempt per tower-destruction epoch and preserve that
    epoch when the equipment is destroyed.

- Dynamic player markers need a marker config entry before `InsertDynamicMarker` can work.
  - `SCR_MapMarkerManagerComponent.InsertDynamicMarker(type, entity, configId)` resolves `type` through the active `SCR_MapMarkerConfig`; stock `CampaignMapMarkerConfig.conf` does not expose every dynamic marker type.
  - Treat custom player markers as optional. Cache a failed `EnsureHSTMarkerConfig()` attempt on the server service and the local-client widget refresh path; otherwise opening the map can repeatedly reload a bad config and spam config errors while also obscuring whether campaign markers are healthy.
  - The custom config must be the marker manager config on every gameplay/dev world layer that should show the marker. Having `Configs/Map/HST_PlayerMapMarkerConfig.conf` in the addon is not enough if the active layer still points at a config without `SCR_EMapMarkerType.HST_PLAYER`.
  - If a world layer still points at the vanilla marker config, patch the active `SCR_MapMarkerManagerComponent` before reconciliation with `EnsureHSTMarkerConfig`, loading `Configs/Map/HST_PlayerMapMarkerConfig.conf` and initializing its dynamic entries. The service should retry and log config readiness instead of silently inserting an unconfigured marker type.
  - Server-side config repair is not enough for dynamic marker rendering. `SCR_MapMarkerEntity.OnCreateMarker()` resolves its widget entry on the client from the local marker manager config, so custom player marker entities must also ensure `HST_PlayerMapMarkerConfig.conf` before vanilla widget creation asks for `SCR_EMapMarkerType.HST_PLAYER`.
  - Validate that the active config still resolves a usable `PLACED_CUSTOM` objective icon before publishing campaign markers. If config repair is needed, load the canonical campaign table and merge only the custom player entry; do not replace a healthy active table merely because the player entry already exists.
  - Do not use `PLACED_CUSTOM` for tracked entities; it is a static marker type. The custom player entry can live in a small source config, while `EnsureHSTMarkerConfig` preserves the canonical placed-marker table and initializes only entries appended after the manager's normal init pass.
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
  - If a key can overlap a native fullscreen UI, gate the mod screen open against the native UI state. The command menu should reject opening while `SCR_MapEntity.GetMapInstance().IsOpen()` is true, but should still allow its own close path if it is already open.
  - Owner-client runtime proof should create the native fullscreen state it needs. For the command-menu map gate, dispatch through `HST_CommandMenuRequestComponent`, open `ChimeraMenuPreset.MapMenu` on the owner client, attempt both `TryToggleCommandMenu()` and `OpenMenuToTab()`, assert the menu stayed closed, and close only the map instance opened by the proof.
  - Current examples: `HST_CommandMenuComponent.RunCampaignDebugMapOpenGateProof()` and `HST_CampaignCoordinatorComponent.BuildCampaignDebugCommandMenuMapOpenGateCase()`.
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
  - For normal gameplay-map target confirmations, parent the prompt, fixed selected-target indicator, and confirmation dialog to `SCR_MapEntity.GetMapMenuRoot()` and keep them in the map-local `7..9` band. The engine pointer remains a workspace child at z-order `10`, so it then renders over the dialog without adding a cursor proxy. Keep the selected-target widgets passive (`IGNORE_CURSOR | NOFOCUS`) and clear them whenever the dialog closes or the player chooses another point.
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
  - Runtime settings schema 19 introduced `civilianDrivingVehicleCountPerTown` and generated comments. Schema 22 changes its shipped true-town default from two to five while preserving non-default values. Use the curated `HST_ZONE_TOWN` classification as runtime authority: much of the valid town catalog still carries imported base-overlay provenance rather than `TC_`/`towns/` metadata. Reclassify a known minor locality explicitly (type, resource kind, garrison, and civilian budget) instead of inferring all older town rows are minor from provenance alone. Minor localities must resolve a zero traffic target rather than multiplying the town budget.
  - Runtime settings schema 21 removes the old arsenal and vehicle loot alias keys. The only user-facing keys are `lootSkipUnlockedItems` / `vehicleLootSkipUnlockedItems`; `true` means skip items that are already unlimited in the arsenal, which is the normal default loot flow. Explosive and guided-launcher unlocks default to enabled.
  - Stock `Character_CIV_*_Randomized.et` resources are editor-facing variant selectors with default randomization disabled. Direct runtime spawning therefore creates the base appearance repeatedly. Build the ambient pool from GUID-qualified concrete stock variants and select them with one zone appearance seed plus the stable actor slot; keep placement/angle seeds separate. Appearance-collision proof must count the complete projected civilian actor set for the locality, including pedestrians and traffic drivers; checking pedestrians alone can miss duplicates introduced by driver replacement.
  - Do not use the hostile-garrison `zone.m_bActive` bit as the sole civilian render condition. A nearby living player should still see town/locality ambience even when military projection is suppressed. Resolve civilian projection eligibility from locality classification plus player distance, while leaving military activation authoritative in the physical-war service.
  - Do not reuse one HQ radius for unrelated policies. The 900 m clearance belongs to hostile operation/QRF staging. Static location activation should be suppressed only when a legacy/emergency HQ lies inside that location's capture footprint (150 m fallback), matching setup placement rules. Composition slots need only an immediate 150 m HQ clearance. A 900 m location-activation exclusion silently erases otherwise valid nearby towns and bases.
  - AI traffic can retain `CharacterInputContext.GetVehicleHorn() != 0` after forced seating/route control. Reset only HST-owned ambient driver inputs with `SetVehicleHorn(0)` after seating and during frame maintenance; do not patch global vehicle prefabs or unrelated AI.
  - Location taxonomy is a gameplay contract, not just map decoration. Towns must remain town/support zones, resources and factories must not be promoted from settlement names, and radar/ferry/dynamic-site pools should be mission-site/bookkeeping targets without start garrisons. Extra zones are acceptable, but curated minimum counts and known IDs should be asserted by Full Campaign Debug preflight.
  - Static location marker icons should not reuse QRF/attacker tactical icons. Keep QRF-style response markers on `OBJECTIVE_MARKER`, publish towns as `OBJECTIVE_MARKER2`, military bases/installations as `FORTIFICATION`, radio towers as `RADIO_SIGNAL`, roadblocks as `JOIN3`, and mission-specific markers by mission family. Phase 23 should assert no static town/base/radio/mission marker has drifted back to a reused tactical icon.
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
  - Per-method complexity can trigger native heap corruption before Workbench
    emits a script error. The observed force-authority proof was 324 lines,
    roughly 25,000 characters, 234 statements, and 38 branches; disabling only
    that method restored the same project's Game compile. Moving the monolith
    intact is not sufficient protection. Keep the coordinator as a thin adapter
    and split deterministic proof fixtures into focused service methods. Current
    example: `HST_ForceAuthorityProofService`.
  - The oversized force-authority proof was removed from the coordinator. The
    latest combined schema-45 Game-module compile/create/script validation passed,
    and a correctly launched normal project open remained responsive through its
    bounded survival check without reproducing the crash. Keep every later
    physical-adapter or debug-proof slice behind its own fresh reload and runtime
    evidence instead of extending the baseline claim by inspection.
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
  - Player-requested QRF/search support should spend HR equal to the planned FIA infantry count. The request state owns `m_iHRCost` and `m_iPlannedInfantryCount`; debug probes should assert the HR delta, money delta, planned count, and composition-derived count before physical routing.
  - Support recall is a command path, not just cleanup. Mark `m_bRecallRequested`, set a recall exit position, route the linked active group through `support_recalling`, and refund HR only after the group reaches `support_recall_exited`, a spawned group passes current live-distance confirmation, and the support tick resolves `recalled_refund_hr`. Refunds are capped by original HR cost and surviving infantry.
  - Return recall authority through `HST_SupportRecallResult`: accepted,
    already-applied, state-changed, terminal, disposition, failure, request, and
    operation identity are independent of `BuildSummary()`. Delay the recall
    flag until routing, cancellation, or settlement is actually accepted. A
    successful physical route must also stamp the request `recall_routing`;
    route failure remains a typed rejection, while a scoped already-ordered
    player request is an idempotent accepted/no-change result.
  - Support cancellation probes should capture the cancellation status/resolution before cleanup, then run player-support cleanup and assert no queued/active player support or cooldown remains. A cancelled row can remain as history, but it must not poison later support requests in the same debug run.
  - For QRF/search player support certification, push the debug-created request to the inbound physicalization window and tick `HST_SupportRequestService` instead of calling abstract completion directly. Assert remaining ETA decreases, runtime status advances, `m_bPhysicalized` flips, `m_sGroupId` is populated, and the linked active group has a support runtime status. Then sample `HST_PhysicalWarService.UpdateRoutedActiveGroupsNow()` across repeated campaign-clock route-state windows and report sample count, movement count, distance-closure count, max movement, max distance closed, and last-observed/stall history. A synchronous campaign-clock jump can prove only that ETA does not create false arrival; actual arrival remains WARN/non-certifying until two distinct-time live samples are observed within 75m during real elapsed time. The existing terminal-injection assertion is conditional on prior live arrival and remains WARN when that prerequisite is absent; only then can it prove `physical_group_terminal` resolution through the real support tick. Keep natural support contact/combat as a separate gap until sampled without forcing the terminal state. Remove/cancel the debug support group/request before the post-case leak probe.
  - Snapshot the linked support marker immediately after the support command and marker rebuild, before the controlled runtime probe advances the request to resolved. Resolved support requests are intentionally omitted by `AddSupportRequestMarkers()`, so sampling marker state only after terminal resolution creates a false WARN even when the marker was published at request time.
  - Route-based physical probes must distinguish campaign-clock samples from real-frame movement samples. The current synchronous ground-support/enemy-order windows record `*.physical_stall_timeout` as WARN evidence even after repeated no-progress rows, because advancing campaign seconds does not advance native AI movement. A hard physical-stall FAIL requires a real elapsed-time sampling window with living-member positions, timeout seconds, last-observed status, and history.
  - Map-target support probes should assert the clicked/requested destination
    remains on `HST_SupportRequestState.m_vTargetPosition`, the linked active
    group keeps that same `m_vTargetPosition`, and deployment summary proves
    offset staging plus player/active-AI clearance before movement samples are
    judged.
  - Roadblock support is not QRF-style movement. Player roadblocks must carry
    selected HQ garage vehicle metadata on `HST_SupportRequestState`, consume
    that `HST_GarageVehicleState` after money/HR spend succeeds, and create an
    established active group at a road-oriented vehicle-safe placement. Debug
    probes should assert garage count delta, `m_bGarageVehicleConsumed`,
    selected vehicle prefab retention on the active group, `roadblock_group`
    physicalization, and an established marker instead of route arrival or
    terminal group resolution.
  - Established roadblocks are support-request markers for all factions. Do not
    rely on resistance-only support-group marker tracking for roadblocks; enemy
    roadblocks are `HST_SUPPORT_ROADBLOCK` support requests linked to active
    groups and should publish native-visible, player-facing support markers
    while active.
  - Route-based physical probes should not fail movement or stall-timeout assertions when the sampled group is already within its applicable target-distance tolerance. Spawned support uses 75m; current generic enemy-order probes use 25m. Distance closure is meaningful only while the group remains outside that boundary.
  - Do not classify `spawn_pending_agents` or `convoy_seating_pending` as a route stall just because the debug runner advanced campaign seconds. Those states depend on real engine/callqueue population or animated boarding; keep them as WARN/pending evidence until the pending state clears. A hard no-progress result still requires a fully sampled real elapsed-time window.
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
  - Phase 18/19 smoke assertions should verify debug-prefixed enemy order/support records, expected types, target zones/positions, resource cost fields, and resolve/ETA behavior. Stage 3 QRF/search support now samples repeated route-state observations, proves a controlled ETA cannot create false arrival, and leaves actual arrival/movement as WARN until real-frame live-position evidence exists. Its terminal-resolution assertion is conditional on prior live arrival and otherwise remains WARN. Keep Phase 19 enemy/support contact behavior as WARN/separate probes until those paths are sampled over time.
  - Phase 18 background-war coverage should seed resistance/occupier/invader POI ownership, top up enemy pools, run the real commander tick threshold, prefix the resulting orders, and assert both occupier and invader order evidence. This proves one controlled command-selection tick; it does not replace long-window escalation or physical advance sampling.
  - Phase 18 background-war commander probes must resolve pre-existing open enemy orders before measuring new order creation. The commander intentionally skips duplicate active orders for a faction/target-zone pair, so stale Phase 17/18 orders can otherwise hide one faction's expected controlled order.
  - Phase 18 background-war probes must not accept or own Petros attack orders. Keep the fixture at ordinary background-war pressure, select POIs outside the HQ threat radius, assert `background_war.commander_tick.no_petros_attack`, and immediately abort any unexpected Petros order so only Phase 22 can create the dynamic Defend Petros mission chain.
  - Phase 24 escalation pressure should reuse the Phase 18 background-war seed, then run separate low/mid/high profiles with exact enemy pool baselines. Capture pool totals before the resource tick, after `HST_EnemyDirectorService.TickResources()`, and after `HST_EnemyCommanderService.Tick()` so resource-income scaling is not confused with commander spending. Resolve open orders between profiles to avoid duplicate-order guards, prefix newly created orders/support/groups, and activate newly created physicalizable order target zones before the physicalization tick so `ShouldPhysicalizeOrder()` has the render-bubble condition the assertion claims to test. Keep group movement as WARN/evidence unless movement is sampled.
  - Phase 24 can cover a short repeated background-war window by running several normal resource/commander tick intervals after seeding the same POI ownership graph. Resolve debug-created open orders between cycles so duplicate-order guards do not hide later commander decisions, retag every created order with the active run prefix, and assert repeated resource ticks, repeated commander ticks, created order counts, and no open prefixed orders after the controlled cycle cleanup. This is stronger than a single commander tick, but it is still not a substitute for an extended autonomous soak across many POIs.
  - Phase 24 pacing seeds are economy/end-state probes, not HQ-threat probes. Reset HQ knowledge/threat/last attack state during those debug seeds so a prior Phase 22 Petros-defense sample cannot spawn a new Defend Petros mission and contaminate pacing cleanup assertions.
  - Aggression decay certification should set a known aggression value on every non-resistance pool, reset the aggression accumulator, tick exactly one configured decay interval, and assert the total decrease equals `decayAmount * enemyPoolCount` without underflow.
  - Phase 20 town support smoke should drive the real `HST_CivilianService.RegisterIncident()` path in both directions, then restore the seeded town state before the runner continues. Assert FIA/occupier support direction, `HST_ZoneState.m_iSupport == clamp(FIA - occupier)`, support/reputation/heat bounds, incident metadata, support-only owner flips, zone marker model owner/color/style reflection, and cleanup restore. With a preset supplied, town incidents flip from enemy ownership to resistance when FIA support is at least 65, FIA leads occupier support by at least 15, and wanted heat is at most 5; resistance-owned towns flip back to an enemy faction when occupier support is at least 65 with a 15-point lead or wanted heat is at least 15 while occupier support exceeds FIA support. The debug probe must restore owner, capture progress, support, heat, incident/security metadata, undercover restriction state, and the rebuilt zone marker model after sampling both directions.
  - Phase 20 wanted-heat smoke should include a copied-state heat-decay probe through `HST_CivilianService.Tick()`. Use a temporary `HST_CampaignState` with copied town/player records, advance it by `HST_CivilianService.HEAT_DECAY_SECONDS`, and assert town heat decreases by exactly one while the town incident timestamp advances. The current player undercover heat behavior cools by one eligible civilian-service tick once the player is not still compromised; record that current behavior directly instead of silently treating it as the same town-window timer. Assert the live campaign town/player records remain unchanged after the copied-state probe. This certifies heat decay logic, not a broader civilian reaction state machine.
  - Phase 20 civilian population smoke should use the scoped `HST_CivilianService.UpdatePhysicalTownPopulationForZone()` helper for a selected inactive town. The global population tick can spawn or clean other active towns, which makes a one-case assertion harder to cleanly restore. Assert live CIV character/vehicle counts, CIV faction tags, spawn radius, spawn-failure delta, pedestrian AI helper coverage, configured traffic count, traffic driver/route helper coverage, bounded movement samples from runtime spawn positions, and cleanup of runtime entities plus transient runtime-vehicle records.
  - A scoped civilian population update returning `false` is not by itself a failed activation. If the runtime town record exists and live CIV runtime entities were spawned, treat the population as active and record the no-delta update as evidence.
  - Civilian population runtime now owns spawn/cleanup/diagnostics plus AI helper assignment for pedestrians and ambient traffic. Movement sample assertions can remain WARN because engine ticks may not move a character inside the short debug window, but missing behavior helpers are a hard failure.
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
  - Phase 23 UI/marker smoke should assert command/menu and marker state directly: no `missing visible command:` or `missing dispatch:` detail rows, compact Missions-tab active rows with no expanded `Where`/`Timer`/`Runtime` detail-row spam, admin menu strings for campaign-debug and Phase-23 controls, every `hst_zone_<zoneId>` marker model entry with linked ID, owner, color, style, and position matching the current `HST_ZoneState`, HQ/mission/support/QRF marker coverage from `HST_CampaignState.m_aMapMarkers`, and marker/backing-state consistency via the same helpers used by post-case cleanup. The native marker report should rebuild markers before reporting, then assert native eligible/published/skipped/failed/pending counters and tracked static handle liveness through structured service getters instead of string parsing. Keep `HST_MapMarkerService` and `HST_CampaignMapMarkerDirector` native marker budgets synchronized; the curated map currently needs more than 96 native-eligible records. Failed-action samples should snapshot state before and after invalid commands so they prove reasoned failure with no campaign mutation. Native map-marker manager absence should be explicit WARN/report evidence; visual widget inspection still requires a separate UI/render probe.
  - Phase 23 Forces UI coverage should keep roadblock support as a map-target
    commander action that is disabled without a stored HQ vehicle, enabled with
    a scrollable/selectable vehicle choice payload, and labeled with HR cost.
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
  - Selectable command prompts should be data-driven by the menu payload, not inferred from already-rendered row text. The commander-transfer member control uses one visible `member_promote_commander_choose` action carrying sanitized SteamID64/name choices, opens the shared scrollable action-choice modal, and dispatches the existing `member_promote_commander` command only after the clicked choice supplies the selected SteamID64.
  - Keep the action-choice modal dynamic and scrollable instead of enforcing a small fixed row count. The same control surface is used by commander transfer and support recall, so payload parsers should accept many sanitized choices and let the scroll host handle visibility.
  - Keep always-visible command menu sections player-facing. Prefabs, raw positions, request ids, group ids, and similar diagnostic handles belong behind explicit report/debug actions, not in the main tab rows players scan during normal command use.
  - Support recall choices should identify candidates by support type, FIA count, deployment status, and relative HQ location. Internal group/request ids can remain hidden action arguments, but Phase 23 should fail if those ids leak into the visible chooser label.
  - Phase 23 command UI coverage should assert the HQ/Petros tab hides prefab, raw position, and attacker-group rows while still exposing useful awareness, threat, defense, and HQ asset summaries.
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
  - A seated convoy driver and `AIGroup.AddWaypoint()` are not enough evidence that the engine vehicle-AI layer owns the spawned vehicle. Resolve the vehicle's `SCR_AIVehicleUsageComponent`, require a valid pilotable usage type, and register it with the crew group's `SCR_AIGroupUtilityComponent.AddUsableVehicle()` before the first seating request; keep the route-time registration as a defensive recheck. The debug suite should query `IsUsableVehicle()` directly and report this separately from driver occupancy, waypoint count, and movement distance.
  - Convoy completion must not infer `all crews eliminated` from missing runtime handles, pending population/seating control, or zero live count during the pending window. Count missing and pending crew groups separately and only complete on elimination when the crew runtime was observable or explicitly terminal.
  - A planned convoy crew count is not an observed living crew count. Keep `m_iLastSeenAliveCount` and survivor counters at zero until live crew is actually counted, and track whether crew was ever alive plus the maximum observed crew so failed population is reported as crew population failure instead of crew elimination.
  - A terminal convoy crew status is not proof by itself. `convoy_eliminated`/`eliminated` should only count toward convoy completion after the crew group has explicit live-crew history (`m_bEverHadLivingCrew` or `m_iMaxObservedCrewAlive`). Do not use preserved last-seen/survivor counters as historical proof; they are bookkeeping. Debug-controlled phase-chain probes may seed that history explicitly; real physical convoys must observe it.
  - A pre-contact crew-count drop is not physical contact proof. Before `convoy_contact`, zero live crew should trigger the convoy-specific repair path or remain `CREW_UNOBSERVED`/pending with explicit evidence; only player contact, destroyed/captured vehicle state, an already-contacted mission, or terminal asset state may make crew loss count as convoy contact/elimination.
  - Convoy reward/outcome application must repeat the same live-history proof. `HST_ConvoyOutcomeService` should not apply `convoy_complete`/`convoy_eliminated` crew rewards from mission event tokens unless matching `mission_convoy_` active groups are eliminated and each group has observed or explicitly seeded live-crew history.
  - Post-completion convoy asset preservation and UI actions must repeat the same live-history proof. Do not preserve or reopen convoy payload, captive, or vehicle interactions from `convoy_complete`/`convoy_secured_sent` tokens alone; pass campaign state into the mission-runtime/UI helpers and require the guarded crew outcome or matching eliminated groups with live-crew history.
  - A campaign-debug convoy physical probe should force a narrow `EnsureMissionConvoyRuntimeNow()` service pass for that mission before asserting physical readiness. Otherwise the probe can sample stale mission state immediately before the normal physical-war tick would have created or repaired runtime handles.
  - Campaign-debug convoy physical case ids should include the runner label/window. The same convoy mission can be sampled during movement/readiness and later during contact; those are distinct evidence windows and should not produce duplicate case ids.
  - Convoy readiness reports must distinguish planned vehicle assets from active/unresolved vehicle assets. A failed or terminal convoy can have planned assets and zero active assets; reporting that as `vehicle assets 0` hides the real failure. Use planned counts for asset-planning assertions and active counts for spawned-vehicle, crew, route, waypoint, faction, and movement thresholds.
  - `SCR_CompartmentAccessComponent.MoveInVehicle()` returning `true` proves only that an owner RPC was dispatched and the slot was locked for the frame; it does not prove an occupant exists. For server-owned convoy AI, call the forced local `GetInVehicle(slotOwner, slot, true, ...)` path first so the same authoritative slice can rescan real occupancy, then use `MoveInVehicle()` when direct local entry is unavailable/rejected or the entity is non-local. If either request is accepted but no driver is visible yet, preserve explicit `convoy_seating_pending` evidence instead of claiming success or reporting an immediate hard failure.

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
- Civilian empty runtime roots are separate from the military empty-root rule. Use a dedicated CIV empty group root for civilian ambience/follow helpers instead of a faction-specific military root, inherit the generic stock group base so the root carries the engine behavior/replication stack, force CIV on spawned character and group components, and attach initial ambient members through `AddAIEntityToGroup()` with a direct `AddAgent()` fallback.
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
  - Visually spawned AI can exist near an active group while the native group card still reports Size 0. Repair this by scanning near the group for matching non-player infantry prefabs, registering the member handles, attaching living agents back to the intended `SCR_AIGroup`, and parenting editable members to the `SCR_EditableGroupComponent` before count or terminal-cleanup decisions.
  - Register dead member handles too. Terminal group cleanup can then detach dead editable children, delete the stale group root/icon, and leave character bodies in the world for loot instead of preserving an immortal empty group card.
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
  - `HST_BuildInfo` is the only owner of build SHA, UTC time, label, and schema identity. Boot logs, admin reports, command-menu readiness, and campaign-debug artifacts must call its runtime-summary helpers instead of defining synchronized component-local constants; mismatched identities make post-test logs impossible to trust.
  - Campaign-debug physical probes should assert the reconciliation result, not only log it. Convoy and physical-combat cases should fail when checkable crew/group/vehicle runtime entities still expose a faction different from `HST_ActiveGroupState.m_sFactionKey`; pending/uncheckable entities can remain WARN while the spawn grace owns them.
  - When direct fallback members are tracked beside their parent `AIGroup`, counting must not double-count or mask a broken native group. Count unique living native-agent members and direct tracked members for faction proof, and keep separate assertions for systems that require the parent `AIGroup` itself to own durable agents.
  - Prefer faction infantry character prefabs from `HST_DefaultCatalog.CreateFactionTemplate(factionKey)` for fallback members. Do not reuse group prefabs or generic FIA defaults for all sides, or server runs can appear to invert/flatten faction composition compared with Workbench expectations.
  - Character liveness checks should read `CharacterControllerComponent.GetLifeState()` first and only use damage-manager state as a fallback for non-character entities. Newly spawned or fallback AI characters can report usable character life state before damage state is meaningful for convoy/physical-war proof.
  - Game Master/editor placement uses the editor path, not the HST physical-war service path. If editor-placed squads only appear after camera movement, inspect streaming/editor placement addons or native editor activation first; do not assume the HST active-group population guard is involved unless HST spawn logs appear around the placement.
- Runtime AI contact probes need a real elapsed-time window. Spawn opposing groups, assign normal `AIGroup.AddWaypoint()` search/destroy or move waypoints, then let the standard service tick sample live counts and distance over time instead of trying to prove combat inside the same script call. Treat waypoint assignment/contact distance as setup/contact evidence and live-count loss as separate casualty-resolution evidence.
  - Physical-combat certification must prove native faction hostility before judging combat behavior. Use `Faction.IsFactionEnemy()` in both directions for the temporary resistance/enemy factions, record that relationship as an assertion, and keep contact distance as the hard runtime proof. Treat no live-count loss after proven hostile contact as WARN because casualty timing is stochastic across short server windows.

- Spawned support route state must come from living runtime members, not elapsed campaign time.
  - The normal-play evidence contained three `physical_arrived` rows whose logged targets and deterministic 2,100m recall-exit vectors imply nominal current positions approximately 434m, 455m, and 505m away. A fourth arrival row implied approximately 30m, so the defect is demonstrated but not universal. ETA is only the earliest time at which arrival may be checked; it is never physical arrival evidence by itself.
  - Resolve a spawned support group's current position from the centroid of living controlled members and preserve that position in active-group state. Elapsed-time interpolation remains appropriate only for an unspawned abstract route.
  - Require two consecutive live-position samples from distinct elapsed seconds within 75m of the current target before changing the active group to its arrived status. A forced/re-entrant second update in the same campaign second is not an independent observation. The support-request service must independently confirm the current live distance before publishing `physical_arrived`. Apply the same live-distance rule to a spawned recall exit.
  - The benign exact QRF deployment status `exact_support_spawn_queued` must normalize to `support_active` at successful handoff; otherwise the physical support route tick does not own the newly realized group. Never normalize every `exact_*` status: `exact_runtime_binding_missing_unresolved` is a force-runtime integrity failure and must remain visible for fail-closed handling.
  - Build spawned support routes directly from the current live position through a safe midpoint to the current target or recall exit. Do not reuse a generated route whose authored direction can lead away from the current leg.
  - Track physical progress separately from campaign state and measure it as net improvement against the best distance seen for the current leg. Centroid motion alone is not progress: lateral movement, backtracking, or oscillation must not reset the stall age or retry budget. A stalled route may reissue its waypoint chain at most three times; reaching the retry cap keeps the group en route and records the missing live arrival instead of inventing completion.
  - Waypoint replacement must be transactional and service-owned. On the server and authoritative group replica, prepare a complete chain first. If fewer than two usable waypoints are prepared, delete only the prepared entities and retain the old route. On success, remove old waypoints from the `AIGroup`, delete their entities, then attach and track the replacement chain.
  - Restore compatibility must reject pre-repair timer truth without disrupting newly proven arrival. An unspawned restored `support_arrived` row folds if it remains outside the player bubble or waits for physicalization inside it. A spawned restored arrival without the new request-and-group proof provenance is rechecked against current live distance, then either stamped as proven or reopened as `support_active`; once proven, arrival remains latched even if combat later moves the centroid away.
  - A Phase 22 group populated 9/9 without observed advance, but the available samples used campaign time rather than repeated physical member positions. Treat it as a reproduction lead, not physical-stall proof. Fresh packaged actual movement, arrival, recall, reissue, and cleanup evidence remains required.

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

## Config-Backed Modded Class Metadata

- A `modded class` that is instantiated from config must retain the base class's
  container attributes. Method inheritance alone is not enough: omitting the
  declaration metadata can make existing native config entries report an unknown
  type even though Enforce script compilation succeeds.
- Preserve the exact base declarations on the current config-backed patches:
  - `SCR_EditorManagerCore` requires `[BaseContainerProps(configRoot: true)]`.
  - `SCR_EditableEntityCoreBudgetSetting` requires `[BaseContainerProps(),
    SCR_BaseContainerCustomTitleEnum(EEditableEntityBudget, "m_BudgetType")]`.
  - `SCR_MarkerIconEntry` requires `[BaseContainerProps(),
    SCR_MapMarkerIconEntryTitle()]`.
  - `SCR_MapMarkerEntryPlaced` requires `[BaseContainerProps(),
    SCR_MapMarkerTitle()]`.
- The missing attributes produced a coherent failure chain in a packaged client:
  editor/budget/placed-marker config types failed to load, the editor manager was
  null, and normal stock HUD/Game Master initialization failed. Player admin and
  commander authority were present, so permission repair was not the primary
  remedy.
- Add static validation for the declaration itself. A headless Workbench compile
  proves the attributes are accepted, but it does not prove the packaged config
  graph deserializes or that HUD/editor ownership initializes. Require a
  republished server/client check with zero unknown types, normal stock HUD, and
  usable Game Master before closing the regression.

## Schema 46 Exact Paid-QRF Authority

- A paid support request needs two durable phases: quote and confirmation.
  `support_qrf` with a map target issues an immutable server quote without
  debiting resources; the confirmation command sends only the quote ID. The
  server revalidates actor, phase, source/target, cooldown, war level, catalog,
  and resources before reserving and committing money/HR.
- The first executable support slice is intentionally one infantry group root
  plus every explicit ordered slot from `HST_ForceCatalogService`. Freeze the
  execution prefab and member prefabs in the manifest. Do not call
  `HST_ForceCompositionService.Compose()` or infer manpower from prefab names
  after quote issue.
- Keep canonical quote/request positions immutable. Queue staging belongs on the
  linked active-group projection and deployment evidence. Overwriting the
  support request's quoted source with a resolved spawn position breaks accepted
  confirmation replay after queue admission because the aggregate no longer
  matches the accepted quote.
- Accepted confirmation is the idempotency anchor. Mark the quote accepted only
  after both resource transactions commit and exactly one support request is
  verified. Replay validation must accept committed, partially refunded, and
  refunded transactions so later failure/recall settlement cannot turn a valid
  accepted replay into an authority conflict.
- Treat strategic service commitment as an explicit settlement boundary. While
  the operation remains `STAGING`, terminal placement/admission failure refunds
  linked money and HR once. Route initialization is allowed in `STAGING`, but
  `LinkOutboundVirtual()` is the final commit to `OUTBOUND`. From that point,
  including `ON_STATION` and a materialization failure with no physical handoff,
  retain the paid money and refund only
  `operation.m_iLastVirtualFriendlyCount` HR. Both sides are idempotent. A
  failed-final batch takes precedence over a simultaneously requested recall;
  otherwise recall could apply the wrong settlement policy.
- A two-resource full refund needs paired preflight before its first mutation.
  Validate both transaction identities, refund bounds/status coherence,
  settleability, and the deterministic settlement-ID replay guard. Otherwise
  money can refund before an invalid or replay-blocked HR leg is discovered.
  This is a bounded paired-settlement rule, not a general rollback mechanism.
  Every exact recall terminal/lost-group branch must inspect settlement success
  separately from diagnostic state change.
- Pre-success recall is distinct from cancellation: it refunds eligible HR, not
  the committed support money. Post-success recall retires the exact runtime
  projection before settling verified survivor HR. The coordinator must pass the
  force-spawn adapter into the normal support tick or retirement can remain
  permanently pending.
- Setup and won/lost frames do not run the normal support lifecycle, so they must
  explicitly inspect exact-support terminal queue state after queue cleanup.
  Settlement is transaction-idempotent and can run while persisted campaign time
  is frozen.
- A durable `SUCCEEDED` queue row is historical evidence, not a live entity
  handle. Schema 46 initially used a temporary full-refund fallback when an
  accepted paid QRF had no provable local binding. Schema 47 supersedes that
  policy with durable survivor reprojection; do not reintroduce entity-ID
  reacquisition or the old full-refund shortcut.
- Legacy player-QRF rows with positive stored cost prove a historical charge but
  not an exact roster. Schema migration may create committed/partially refunded
  transaction history without changing balances; it must not invent a quote,
  manifest, new debit, or refund. Conflicting transaction identity stays
  preserved with one bounded warning.
- Campaign-debug QRF lifecycle probes that still need broad-alpha behavior must
  use an explicitly named debug-only legacy wrapper and state that they are not
  exact-authority proof. Production resistance QRF requests fail closed unless
  they enter through an accepted server quote.
- Headless Workbench validation needs all addon roots explicitly discoverable.
  Use `-addonsDir` with the base-game, Workbench, and user addon directories in
  addition to the quoted `-gproj` argument; otherwise the project may appear to
  fail because its base-game dependency cannot be found before scripts compile.
- The schema-46 Game module loaded 5,736 files/11,460 classes, created the game,
  and completed script validation. A separate normal WorldEditor open stayed
  responsive through 20 seconds and did not reproduce the earlier Workbench
  crash. These are compile/startup checks, not physical paid-QRF runtime proof.

## Schema 47 Exact Force-Runtime Lifecycle

- Persist lifecycle facts separately from process-local physical evidence. An
  exact member slot retains `everAlive`, casualty confirmation/time/reason, and
  its lifecycle revision. The batch retains successful-handoff and reprojection
  counts; the active group retains `everPopulated`, `spawnCompleted`, durable
  living infantry, and elimination time.
- Do not infer death from a missing or deleted entity. The adapter samples the
  still-present, exact slot-mapped entity's authoritative character life state
  before pruning deleted transient handles. A later event-driven subscription
  can replace polling, but it must preserve the same slot identity and
  idempotency contract.
- A confirmed dead member transitions from `REGISTERED` to `RETIRED` exactly
  once. Detach its native agent and Game Master parent and remove it from force
  runtime ownership without deleting the corpse. This makes native/GM/strategic
  living strength converge while leaving loot/salvage cleanup to its own policy.
- Last-death cleanup is guarded by both `everPopulated` and `spawnCompleted`.
  When durable living count reaches zero and no runtime member remains alive,
  delete the exact root, clear its runtime handles/waypoints, mark the active
  group eliminated, and let support settlement remove the active-group marker.
  Money and dead HR remain committed.
- Restore never reacquires a saved entity/native-group ID. Clear physical IDs,
  retain `RETIRED` casualty tombstones, requeue the same root plus only member
  slots with ever-alive and no confirmed casualty, then require a fresh exact
  handoff. The immutable manifest stays unchanged; the queue's resolved-slot
  check treats a valid retired member tombstone as complete but nonphysical.
- A failure before strategic service commitment keeps the full money/HR refund
  policy. `successfulHandoffCount > 0` remains durable proof of physical
  delivery, but it is no longer the earliest delivery boundary: an exact held
  force is already delivered as a virtual service after `OUTBOUND`. A later
  no-handoff materialization failure therefore retains money and refunds the
  operation's virtual survivor count; a post-handoff failure uses the durable
  slot survivor count. If required queue authority is unavailable after a
  handoff, settlement waits instead of inventing a full refund. This distinction
  must survive save/restart.
- Pre-schema-47 successful batches backfill one historical handoff and ever-
  alive evidence from registered alive slots. Linked active groups receive
  spawn-completed/ever-populated plus the exact registered living count. Do not
  invent casualty slots from lower aggregate survivor fields.
- The bounded `force_runtime.*` proof exercises casualty replay, lifecycle deep-
  copy, retired-slot exclusion during survivor reprojection, second handoff,
  final durable-zero roster, and schema-46 migration. It does not replace a live
  runtime proof of corpse detachment, native/GM sizes, five-second root/marker
  removal, or a real process restart.
- The schema-47 Game module currently loads 5,737 files/11,462 classes, creates
  the game, and completes script validation. A separate normal WorldEditor open
  remained responsive at every two-second sample through 20 seconds and did not
  reproduce the earlier crash. These remain compile/startup evidence until the
  isolated physical and restart cases run.

- Schema-48 accepted-settlement archives are intentionally separate from exact
  member casualty tombstones.
  - Archive only `ACCEPTED` quotes whose consumer has a provable terminal
    settlement, whose linked transactions are unique and non-reserved, and whose
    manifest has no active-group, enemy-order, or force-spawn backlink. Ambiguity
    keeps full rows; it is never a reason to infer settlement.
  - Preserve command/confirmation IDs, actor, operation, quote/manifest/hash,
    aggregate ID, counts/costs, final transaction status, refunded amount, and
    settlement ID. Reconstruct read-only quote/manifest/transaction objects only
    for replay summaries and integrity checks; never put them back into full
    campaign arrays.
  - Use a 600-second full-row minimum, 86,400-second tombstone replay minimum,
    256 tombstone rows, and 320 combined full/tombstone planning rows. When
    protected history fills the bound, reject new planning rather than deleting
    a live backlink or young replay row.
  - Schema-47 migration initializes an empty archive and retains every accepted
    row in full. It must not infer a settlement merely because an old request is
    missing process-local runtime handles.
  - The final schema-48 Game module loads 5,739 files/11,472 classes and creates
    the game. A separate normal WorldEditor open remained responsive at every
    two-second sample through 20 seconds and did not reproduce the earlier crash.
    This is compile/startup evidence, not archive replay or restart evidence.
  - The typed support-recall follow-up keeps schema 48, loads 5,740 files/11,477
    classes with CRC `34f7b92b`, and creates the game under headless Workbench validation. Its
    deterministic proofs cover explicit receipt status, paired preflight
    rejection, and lost-group settlement; they are not packaged RPC,
    save/restart, or physical recall-exit evidence. A separate normal
    WorldEditor open remained responsive through ten two-second samples with no
    new script-error or crash signature.

## Schema 49 Exact Paid-QRF Operation Authority

- Operation identity may exist before an operation aggregate. Exact QRF quote
  issue persists the stable operation ID in planning authority but must not
  create an `HST_OperationRecordState`; a quote can expire or be cancelled
  without ever becoming an operation. Register exactly one record only after the
  exact support confirmation has successfully created and verified its accepted
  request/quote/manifest authority.
- Opt-in is explicit and versioned. New confirmed exact paid player infantry QRF
  requests set operation contract version `1`. A request on contract `0` follows
  its legacy behavior and must not be forced through operation validation merely
  because it carries a historical operation ID.
- Keep immutable assignment separate from mutable tactical intent. Origin and
  `support_on_station` assignment come from the accepted quote and never change.
  Arrival uses that assignment; recall changes only the tactical target toward
  the exit/origin. This prevents an exit waypoint from rewriting why and where
  the force was originally committed.
- Treat duty, engagement, materialization, position authority, and settlement as
  orthogonal state. Queue admission advances duty to `OUTBOUND` and
  materialization to `MATERIALIZING`; successful handoff makes live physical
  state authoritative; arrival advances duty to `ON_STATION`; recall records
  request/exiting duty; settlement closes duty, materialization, engagement, and
  terminal result together.
- Lifecycle callbacks can replay late, so forward transitions need legal source-
  state guards and link replacement checks. In particular, a duplicate outbound
  materialization callback may replay while an operation is already physical or
  on station; reject it without changing duty, position authority, revision, or
  execution links instead of regressing the operation to strategic outbound.
- Engagement is a domain API, not inferred prose. The legal cycle is
  `CLEAR -> CONTACT -> ENGAGED -> DISENGAGING -> CLEAR`, with the current duty
  copied into a resumable field when contact begins. Reject illegal or post-
  settlement transitions without mutation. Schema 49 does not yet subscribe to
  live combat contact/disengagement, so do not describe this API as natural
  combat behavior.
- Every terminal ledger path must preflight and then settle the operation with a
  deterministic settlement ID and typed result. Exact replay of the same result/
  ID is a no-op; a conflicting result or ID fails closed. Current results include
  recalled, spawn failed, destroyed, cancelled, and invalidated. Operation
  settlement complements resource settlement; neither one makes an unvalidated
  mutation by itself.
- Process-local physical authority never survives restore. Deep-copy every
  operation field, but normalize an open saved `PHYSICAL` or `DEMATERIALIZING`
  record to strategic `MATERIALIZING`, preserve duty and immutable assignment,
  and wait for exact survivor reprojection before returning to live authority.
- Schema-48 migration is conservative. Backfill only an accepted nonterminal
  exact paid player QRF whose support request, accepted quote, immutable manifest,
  unique committed money/HR transactions, and any optional spawn batch/active
  group are mutually coherent.
  Do not change economy, transactions, quote/request status, queue status, group
  status, or balances. Pre-exact, terminal, tombstone-only, incomplete,
  ambiguous, legacy/enemy, and other-support rows remain contract version `0`.
- Bounded settlement archival owns terminal history. A contract-version-1 paid
  QRF may compact only when its operation is coherently settled and all existing
  backlink rules pass. Copy operation contract version, settlement ID, revision,
  and typed terminal result into the tombstone, then remove the full settled
  operation record with the other full aggregate rows. Tombstone replay must not
  recreate a full operation.
- Scope claims must stay narrow. Schema 49 does not provide strategic route
  cursor/hysteresis, generalized virtual/physical transfer, vehicles/assets/
  multi-root force operations, other paid support, garrisons, missions, enemy
  orders, legacy `HST_QRFState` conversion, live engagement events, or client/JIP
  operation projection. It is the first operation kernel, not full Phase 4.
- The stamped schema-49 source passes foundation validation, loads 5,743 files/
  11,497 classes, and creates the game with CRC `4efe34fc`. A fresh normal
  WorldEditor project open stays responsive through the bounded 20-second gate
  without an unknown-config-class or crash signature. This is source/compile/
  startup evidence, not packaged runtime proof. `HST_OperationRecordProofService`
  contributes eight stable
  `operation_record.*` assertions to the existing force-authority campaign-debug
  case and covers issue/confirm, materialization, engagement, recall/settlement,
  restore, migration, archive, and legacy-QRF isolation. The proof is implemented
  and compiles, but packaged execution and real save/restart/migration/archive
  replay are still required.
- That successful compile/open belongs only to the stamped schema-49 revision. Do
  not carry it forward as validation for schema 50 or any later source change.

## Schema 50 Exact Infantry-QRF Strategic Projection

- Campaign travel owns a versioned route segment, cursor, speed, and last-update
  clock. Derive ETA with `ceil(distance / speed)`, advance only campaign time,
  and cap catch-up work per tick; a player render bubble decides projection mode,
  not whether the strategic force continues moving.
- A physical-to-virtual fold is an authority transfer, not a casualty. Preserve
  exact retired casualty slots, requeue only living manifest slots, rebase the
  remaining route at the last live position, and reset the virtual-combat clock
  at the fold second so physical time cannot be resolved twice.
- Treat materialization and dematerialization as retryable multi-step transfers.
  If queue release succeeds but the operation rejects materialization, put the
  pending batch back on strategic hold before it can spawn. If runtime retirement,
  survivor requeue, or fold completion pauses after `DEMATERIALIZING`, resume that
  same transfer before normal batch dispatch on the next tick; never strand the
  operation between live and strategic authority.
- Evaluate player proximity before advancing virtual combat. Once a living player
  is inside materialize-in distance, queue the exact current survivor roster for
  physical handoff without also resolving another abstract combat step on that
  tick. Conversely, every virtual engagement/clock/damage-carry mutation must
  report `stateChanged`, even when no casualty threshold is crossed.
- Restore has the same one-owner rule. A previously successful physical batch
  becomes one held pending projection and increments its reprojection diagnostic
  once; an already-held pending batch does not. Rebase route state at the saved
  group position before normal route initialization to prevent origin snapback.
- Normalize the linked support request at that same boundary. Clear
  `m_bPhysicalized`; an on-station operation remains abstract-resolved with
  `exact_virtual_on_station`, while other open duties resume as
  `exact_restore_survivor_virtual`. Normalizing only the operation and batch
  leaves a contradictory request that can advertise nonexistent physical
  authority after restart.
- Until physical contact callbacks own engagement, a successful virtual-to-
  physical handoff must legally walk abstract contact through disengaging to
  `CLEAR`. This makes the larger materialize-out boundary foldable; virtual
  combat will deterministically reacquire target-garrison contact after folding.
- The current virtual-combat consumer is intentionally narrow: one exact paid
  infantry QRF versus the hostile target-zone infantry garrison, in bounded
  deterministic 30-second steps. Vehicles, assets, multi-root forces, broader
  encounters, and capture resolution still fail closed or remain on prior paths.
- Route validation occurs while the operation is `STAGING`; `LinkOutboundVirtual`
  is the durable strategic-service commit boundary. A failure before that link is
  eligible for the original full money/HR refund. A terminal materialization
  failure after `OUTBOUND`/`ON_STATION` retains the spent service money and refunds
  only the exact last virtual survivors' HR, with the normal settlement ID making
  replay idempotent.
- The schema-50 source passed foundation validation and Workbench script
  validation. The Game module loads 5,747 files/11,508 classes with CRC
  `16665f19`, and the correctly quoted normal WorldEditor project open remains
  responsive through ten two-second samples without a script-error,
  unknown-class, or crash signature. These gates prove source compile/startup,
  not packaged movement, projection, combat, or restart behavior.

## Schema 51 Exact Enemy Defensive-QRF Authority

- Opt an enemy order into exact operation authority before any legacy runtime
  path can see it.
  - Only a newly planned `HST_ENEMY_ORDER_QRF` with operation contract version
    `1` is eligible. Historical rows remain version `0`; migration must not
    infer a source zone, roster, operation, service commitment, or refund from a
    legacy order ID.
  - `HST_EnemyCommanderService` delegates a versioned row to
    `HST_EnemyQRFOperationService` during its active tick and excludes it from
    the fixed resolve timer. `HST_PhysicalWarService` also declines to create a
    spontaneous legacy QRF while an open exact operation owns the same faction/
    target pair. Suppressing only one of those paths still permits duplicate
    strategic or physical responses.

- Freeze the infantry roster before committing the enemy debit, then make
  post-debit admission all-or-nothing.
  - Select a distinct same-faction source and build one deterministic manifest
    containing exactly one supported group root and its authored member slots.
    Link source, target, faction, costs, accepted count, and manifest hash back
    to the order.
  - Treat hideout and mission-site anchors as bookkeeping, not operational QRF
    sources. Validate the responding faction through faction-relation authority
    and require the defended target to remain owned by that same hostile faction,
    so a neutral or resistance target cannot receive an exact defensive response.
  - After the attack/support debit succeeds, admission must create exactly one
    reciprocal order/operation/manifest/batch/group aggregate and place the
    batch on strategic hold. Any later admission failure cancels partial queue
    work, removes unpublished rows where safe, and applies one deterministic
    full-refund settlement. A replay with different settlement evidence fails
    closed.
  - Run the queue's shared, read-only identity/manifest/capacity validator before
    `TrySpendDefense()`. Repeating weaker checks in the caller is insufficient:
    a rejection after spend restores resource totals but can otherwise consume
    the enemy-support cooldown. `CanEnqueue()` and `Enqueue()` must share the
    same validator, and a committed-order replay must return the persisted
    frozen manifest instead of replanning from a later war level or timestamp.
  - Current examples: `HST_ForcePlanningService.PlanExactEnemyDefensiveQRF()`,
    `HST_EnemyQRFOperationService.AdmitPreparedOrder()`, and
    `FailAdmissionAfterDebit()`.

- A projection transfer is not a resource-settlement boundary.
  - Outbound and return legs reuse the same persisted direct-route cursor,
    strategic hold, materialize-in/out hysteresis, exact slot retirement, and
    survivor requeue contract as the exact player infantry QRF.
  - Folding physical entities outside the larger player radius changes only
    position/roster authority. It must not apply either a full refund or a
    survivor refund. Confirmed dead slots remain retired and only living slots
    can rematerialize.
  - On terminal completion or invalidation after service commitment, refund
    `cost * survivors / acceptedMembers` for each enemy resource pool exactly
    once. Admission failure before commitment refunds the full recorded debit;
    destruction with zero survivors refunds zero.

- Physical arrival needs durable observations but not durable process-local
  confidence.
  - Target arrival and return-to-origin require two in-radius live-position
    confirmations from distinct campaign seconds. Reset the count whenever the
    route leg changes so an outbound sample cannot satisfy the return leg.
  - Persist the fields for save consistency, then clear them on restore. A
    process-restart cannot replay a prior physical sample as current entity
    evidence.
  - Virtual arrival comes from the persisted strategic cursor and therefore
    does not need physical live-position samples.
  - Restore an already-virtual route without rebasing its start or zeroing its
    progress. A saved physical leg may adopt the last live position, but an
    endpoint reached with fewer than two confirmations must resume just outside
    the arrival radius so one transient sample cannot become a virtual arrival.

- Separate arrival outcome from terminal settlement.
  - `m_bOutcomeApplied` makes the defensive capture-pressure reduction a
    once-only target-arrival effect. If ownership changed before arrival, record
    the no-pressure outcome instead of mutating the new owner's state.
  - Arrival advances to `ON_STATION`, then rebases the tactical route to the
    immutable origin and enters `RETURNING_TO_ORIGIN`. Only origin arrival
    settles `COMPLETED`; a fixed elapsed-time resolution is not authoritative.

- Restore exact enemy QRFs as one coherent virtual aggregate.
  - Validate reciprocal order/operation/manifest/batch/group IDs, manifest hash,
    faction, source, target, service commitment, and resource-settlement
    evidence before normalizing anything. The active group's enemy-order
    backlink keeps generic restore code from reclassifying the projection as a
    garrison.
  - Clear root/member/native-group handles and physical flags, retain route
    progress and exact casualties, and normalize one nonterminal batch to held
    virtual authority. Incomplete or conflicting versioned authority is
    invalidated and settled through the exact owner; it must not fall back to a
    legacy support request or legacy QRF row.
  - Unknown committed survivor authority is zero, not the original accepted
    roster. Validate the settlement kind, accepted/survivor counts, calculated
    refund amounts, and matching operation receipt before reusing an applied
    refund. Keep casualty rows while that validation is retryable.
  - A settled operation is an archive receipt: its batch and active-group rows
    may already be gone. Current-schema restore requires runtime backlinks only
    for an open operation and must preserve `RESOLVED` plus its original terminal
    result across save/load after runtime cleanup.
  - Choose full-versus-survivor restore settlement from durable commitment
    evidence, not from one flag alone. Any durable execution link that proves
    work crossed the admission boundary forces conservative survivor-mode
    settlement; only the coherent canonical aggregate may resume execution.
    Missing or ambiguous links never justify a full refund. Preserve conflicting
    rows as evidence instead of deleting them to manufacture uniqueness.
  - Treat secondary order/operation/manifest/batch/group backlinks as ownership
    evidence when counting ambiguity, even when a conflicting row has a different
    primary ID. Do not normalize a conflicting backlink before validation.
  - A partial resource receipt or a terminal operation settled under a different
    receipt is quarantine evidence, not permission to overwrite the fields or
    issue another refund. Leave that row blocked for diagnosis.
  - Reconcile only after campaign foundation has recreated faction resource
    pools. A restore-time refund cannot be applied safely against a missing pool,
    and postponing pool creation until after exact reconciliation can strand a
    terminal operation with unsettled resources.

- Runtime failures must terminate through the exact owner rather than leave an
  open operation stalled indefinitely.
  - Route initialization, virtual advance, live-route restart, missing runtime
    binding, and failed/non-successful physical batches need an explicit typed
    failure settlement. Retire any physical handles first, preserve the last
    provable survivor count, then settle once.
  - Run a pure terminal-eligibility preflight first, then apply the canonical
    enemy-resource settlement, then commit the operation terminal transition.
    Restore/replay may encounter resources already settled but order backlinks
    not finalized; finish cleanup idempotently instead of charging or refunding
    again.
  - Terminal row cleanup is complete only when adapter bindings, the owned
    PhysicalWar root, and owned runtime members are all absent. If reciprocal
    runtime links disagree, quarantine the order without reading that roster,
    refunding it, or retiring the possibly foreign projection. Likewise, a
    zero-slot roster becomes `DESTROYED` only after the physical elimination
    finalizer confirms that no live exact member remains.
  - A production exact row never downgrades to contract version `0`. If a debug
    fixture needs the historical timer path, seed it through an explicitly named
    debug-only legacy helper.

- Operation markers should consume operation authority, not recreate it.
  - Publish one marker for each open exact enemy defensive-QRF operation at the
    strategic cursor while virtual and the live group centroid while physical.
    The label can derive faction, living slot count, duty, the current outbound
    or return leg, and ETA without creating a second QRF lifecycle record.
  - The expected-marker census must use the same order/operation eligibility
    predicate as publication. Counting an open operation whose order was already
    invalidated creates a false coverage failure and can hide a real stale marker.
  - An authoritative batch roster count of zero is still authoritative. Do not
    replace it with stale positive active-group survivor fields merely because
    zero was also used as the old "unknown" sentinel.

- `HST_EnemyQRFOperationProofService` contributes six deterministic
  `enemy_qrf.*` assertions: admission, legacy isolation, projection,
  settlement, persistence, and rejection. Persistence and rejection now include
  refund-only partial receipts, missing current-schema group backlinks, and
  missing-canonical/shadow committed-replay claimants. The final stamped
  schema-51 tree passes foundation validation; its headless Workbench run
  compiles 5,749 Game files/11,516 classes and creates the game with CRC
  `85ccf2e0` without a script error. At that schema-51 checkpoint, the available
  normal WorldEditor evidence still belonged to schema 50;
  it loaded the same file/class counts, created the game with CRC `a8dad007`, and
  remained responsive for all ten two-second samples. These source/startup gates do not prove
  physical movement, AI casualties, marker rendering, packaged accounting, or a
  real process restart.

## Crewless Mixed Active-Group Lifecycle

- Keep personnel and vehicle liveness separate. `CountAliveRuntimeGroupAgents()`
  remains the aggregate helper needed by intentional vehicle-only projections,
  while `CountAliveRuntimeInfantryGroupAgents()` is the combat/terminal authority
  for a group whose durable plan contains both infantry and vehicles. Do not fix
  the mixed-QRF defect by globally treating every uncrewed vehicle projection as
  dead.
- A mixed group may become personnel-terminal only after it has positive prior
  population evidence and both native delayed-population and live-count grace
  have expired. This prevents a newly created empty `SCR_AIGroup` shell from
  being mistaken for combat casualties. Queue-owned exact projections and
  mission convoys retain their independent lifecycle authorities.
- On confirmed zero living personnel, apply one terminal transition: set
  `eliminated`, zero spawned/living/survivor/durable-strength counts, clear
  runtime identity and waypoint count, stamp casualty/elimination time and
  lifecycle revision, and fail any still-unresolved linked QRF. Replay must be a
  no-op.
- An intact attached vehicle is neutral disposable field salvage, not a living
  enemy member, a garrison refund, or an automatic garage reward. Clear its
  recursive faction claim, register it as `detached_active_vehicle` using
  `m_sVehiclePrefab` before any group-prefab fallback, unregister the group-to-
  vehicle handle without deleting the physical root, and require explicit
  player garage capture for durable ownership. `m_bDetached = true` deliberately
  prevents automatic field-vehicle restoration if the session ends first.
  `HST_CampaignSaveData` excludes these session-only records and their virtual
  cargo on capture and prunes legacy copies on migration, so they cannot become
  unbounded stale runtime-ID aliases after restart. Campaign-debug isolation
  comparisons intentionally ignore this same transient record class because
  the persisted deep-clone boundary excludes it while the untouched live world
  may still own its session salvage entity.
- Do not downgrade a vehicle that was already adopted as durable
  `loot_vehicle`, `field_vehicle`, or `garage_redeploy` state. Preserve that
  record's kind, `m_bDetached = false`, cargo, and save/restore policy while
  removing only its active-group combat ownership. Only a vehicle without an
  existing persistent field record becomes session-only detached salvage.
- Destroyed or missing attached vehicles are unregistered/deleted normally.
  Terminal cleanup is idempotent and removes both runtime-vehicle arrays in
  reverse order. The outside-bubble support fold calls the same crewless mixed-
  group guard before it can abstract-return dead personnel or an abandoned
  vehicle.
- QRF marker eligibility must consult the linked group's terminal status before
  the unresolved-QRF shortcut. Zone capture already ignores terminal groups, so
  zeroing survivor vehicle strength removes both stale capture pressure and the
  tactical marker without weakening a genuinely living QRF during an ownership
  flip.
- Save normalization must never backfill original infantry/vehicle counts into
  an `eliminated`, `convoy_eliminated`, or `spawn_failed` row. Those terminal
  rows restore with zero strategic survivors and no process-local runtime
  identity. A `folded` row is different: it must retain the exact survivor
  counts already credited to its abstract garrison. This repair changes no
  persisted shape and therefore keeps schema 48.
- Neutral salvage is gated by the explicit
  `personnel_eliminated_vehicle_salvage` disposition. Generic `folded`,
  `spawn_failed`, and other terminal cleanup must delete/unregister its vehicle
  normally; preserving it after fold-back would duplicate the vehicle between
  the abstract garrison and the world.
- `HST_ActiveGroupLifecycleProofService` exercises the production state
  eligibility predicate and terminal mutation, including living, pending,
  queue-owned, convoy, unobserved, delayed-population, and live-grace controls.
  It also covers linked QRF failure, capture-pressure removal, marker ordering,
  replay idempotence, current-schema roundtrip, folded-survivor preservation,
  session-salvage pruning, living-mixed control, and vehicle-only control. It is
  deterministic in-process proof; real entity detachment, player salvage,
  replication, and process restart still require a disposable packaged runtime
  run.
- The final stamped lifecycle build identifies implementation SHA
  `3157ca28b066630ffb87cac292f74e20ce243efd`, loads 5,741 files/11,481 classes,
  creates the game with CRC `077afac2`, and survives a correctly quoted normal
  WorldEditor open through ten responsive two-second samples. These gates prove
  compilation/startup only, not the real entity/salvage behavior above.

## Campaign Debug Observation Timing

- A debug assertion must consume evidence captured at the state transition it
  claims to prove. Marker icon, active-group target, member counts, editable
  size, native RUN movement, and formation evidence must be copied before the
  probe intentionally tears down a marker, folds a group, or removes its runtime
  entity. A later lookup can only be a fallback when no observation was captured.
- Enfusion-formatted bool fields can appear as `true` or `1`. Use one normalized
  report parser for both representations instead of matching a single literal.
- Formula expectations and human-readable reports must describe the same state.
  For a forced economy tick, calculate expected income and capture the inspection
  report before applying the tick, then compare the post-tick resource delta to
  that shared pre-tick evidence.
- Aggregate case status and headline reason by explicit severity. A later FAIL
  must replace an earlier WARN reason; the intended order is FAIL, BLOCKED, WARN,
  SKIPPED, then PASS.
- Taxonomy minimums must follow the authoritative curated registry. The resource
  pool intentionally contains 12 sites; an assertion for 13 creates a permanent
  harness failure without increasing gameplay coverage.
- After these six observation repairs, the Game module still loads 5,739 files/
  11,472 classes and creates the game. A separate normal WorldEditor project open
  stayed responsive at every two-second sample through 20 seconds with no
  script-error or crash signature. The corrected assertions remain runtime-open
  until a fresh isolated Full Campaign Debug artifact executes them.

## Schema 52 Exact Mission-Convoy Authority

- Keep the fail-closed policy and save-graph validator focused.
  - `HST_MissionConvoyP1Policy` owns mission-specific cargo cardinality,
    role/kind/entity-prefab admission and the legal duty/resume plus settlement/
    materialization/position pairs. Require every cargo prefab to expose the
    mission-asset component. A captive must inherit the character root and
    expose compartment access because projection boards it through that
    component; a normal payload must remain a non-character entity.
  - `HST_MissionConvoySaveValidationService` owns conservative migration,
    current-schema claimant validation, quarantine, derived-survivor
    normalization, and restore cleanup. `HST_CampaignSaveData` should remain a
    thin orchestration/deep-copy container instead of absorbing that validator.

- Opt-in is explicit and forward-only.
  - Only a newly started `convoy_intercept` mission receives operation contract
    version `1`. A restored historical convoy stays contract version `0` and
    keeps the legacy timer/physical-runtime path.
  - Never migrate a historical row by inventing a route, operation, manifest,
    vehicle slot, crew roster, cargo carrier, convoy element, or settlement
    receipt. Unknown historical identity is not exact authority.

- Plan the physical mission assets before admitting exact authority.
  - Admission requires exactly three complete `convoy_vehicle` mission assets
    and one persisted generated road route with a usable polyline. Missing or
    partial input fails the mission contract instead of creating a shortened
    convoy.
  - Freeze three vehicle slots, one reciprocal crew-group slot per vehicle, and
    every ordered crew-member slot. The first member is the driver and all
    member/vehicle relationships use stable slot IDs rather than array order.
  - Freeze at most one convoy cargo/captive asset and assign it to vehicle slot
    zero. Admission must prove the optional row has the exact mission-compatible
    role/kind and a nonempty, loadable mission-asset entity prefab before
    freezing it. Enforce the boardable-character/non-character distinction for
    captive versus ordinary payload rows here and again during restore. While it
    remains unresolved, project its current/last-known position from that carrier
    element; do not select a different vehicle after restore.
  - Admission rollback must remove only the deterministic rows prepared by that
    attempt, clear only matching asset backlinks, and reset the uncommitted
    mission contract/IDs to version `0`. Asset retention must require a complete
    reciprocal version-1 operation/manifest/batch graph; a failed preflight is
    not durable exact authority.

- Treat the held spawn-result row as durable roster evidence, not generic
  SpawnQueue vehicle execution.
  - Group, vehicle, member, and optional asset slots are registered in one held
    batch so virtual living/casualty authority survives projection changes.
  - The generic adapter remains limited to its exact single-infantry-root shape.
    `HST_PhysicalWarService` is the narrow schema-52 engine adapter for the three
    convoy vehicle/crew elements and must exclude those held groups from legacy
    queue and convoy population ownership.
  - Generic queue acquisition, duplicate resolution, restore reconciliation,
    terminal cancellation, and compaction must recognize the exact convoy
    policy and deterministic IDs before mutating a row. An exact or quarantined
    convoy batch is roster evidence owned only by the convoy operation service.

- One convoy element is the durable authority for each vehicle/crew pair.
  - Persist reciprocal operation, mission, manifest vehicle slot, vehicle asset,
    crew-group element, active-group, and cargo IDs plus formation/current
    position, original/surviving crew, vehicle damage/fuel/ammunition snapshot,
    disposition, physical/mobile flags, terminal reason, update second, and
    revision.
  - `ACTIVE`, recoverable crewless `ABANDONED`, `DESTROYED`, `CAPTURED`, and
    `ARRIVED` are durable dispositions. Settlement converts remaining active or
    abandoned rows to `RETIRED`. A missing runtime handle is never proof of
    destruction, capture, or crew casualty.

- Virtual travel follows the persisted generated-route polyline.
  - Advance the lead cursor at 9 m/s, cap one catch-up invocation at 900 campaign
    seconds, and project trailing elements 22 m apart. The operation cursor and
    element positions—not the old mission timer—own ETA and off-screen travel.
  - The staging countdown may use its authored delay, but once outbound the
    frozen route's remaining distance owns ETA in virtual, materializing,
    physical, and contact states; never let legacy counter C overwrite it.
  - This slice does not simulate off-screen combat. Virtual state preserves the
    latest authoritative crew/vehicle condition; it does not invent casualties
    merely because time passed outside the render bubble.

- Materialization is hysteretic and fail-closed.
  - Begin materialization when a living player is within 1,800 m of the exact
    convoy; consider folding only beyond 2,200 m and after at least 60 seconds of
    physical ownership. A 180-second incomplete materialization fails the exact
    mission rather than recording underpopulation as combat loss.
    Measure the bubble against the nearest separated living element, not only
    the aggregate operation marker. In every open phase, include each recoverable
    abandoned vehicle and unresolved cargo root as well; a partially abandoned
    vehicle beside a player must not fold merely because the surviving column moved.
  - During outbound travel, materialize active vehicle/crew roots at their
    durable current positions using the exact identities. If a vehicle is
    durably `DESTROYED` or `CAPTURED` while crew remain alive, materialize only
    that frozen surviving crew at the element position; require zero runtime
    vehicle identity so rematerialization cannot resurrect the terminal root.
    An intact unresolved crewless vehicle may reproject for player recovery even
    while other convoy roots are still outbound, and it remains eligible during
    the later on-station recovery hold. Its zero-survivor crew is never repaired
    or resurrected.
  - Outbound materialization is one transaction across all three separated
    roots. Snapshot the transient durable fields before the first spawn and
    unwind every root created by that attempt if any root reaches a terminal
    spawn failure. Exact convoy crew creation is synchronous from frozen member
    slots and bypasses generic delayed population. Do not expose a partial exact
    convoy until all required roots, mapped members, and cargo are ready.
  - Staging flags apply to every concrete participant, not only the group and
    vehicle roots. Each exact member entity is independently inactive, invisible,
    and non-traceable while the transaction is open. Commit must revalidate cargo
    in the same call, publish cargo plus every root/member, verify all publication
    flags, and only then remove the transaction. Durable `PHYSICAL` state alone
    is not permission for PhysicalWar to close a cargo-blind transaction.
  - Apply the same publication rule to crewless abandoned vehicles. They may
    exist internally while `MATERIALIZING`, but remain inactive, invisible, and
    non-traceable until the whole exact projection reaches `PHYSICAL`; rollback
    removes every unpublished root.
  - Cargo/captives remain bound to their frozen vehicle slot while that carrier
    is intact. If the exact carrier becomes `DESTROYED` or `CAPTURED`, preserve
    its last durable position and let MissionRuntime project the unresolved
    asset there as a nearby-player ground recovery regardless of whether that
    carrier's crew survived; never migrate it to another convoy vehicle.
  - Physical readiness requires every materializable active element to have its
    owned vehicle/group and exact survivors, every eligible terminal survivor
    to have its crew-only root and no vehicle, and unresolved cargo to have a
    real current projection rather than stale persisted spawned flags.
    MissionRuntime must project frozen-carrier cargo while the operation is
    `MATERIALIZING`, before the readiness check; waiting for `PHYSICAL` creates
    a circular handoff that can only end in the watchdog.
  - Partial materialization is not casualty evidence. A group in native-agent
    population grace, AI-world budget deferral, or spawn failure can publish
    temporary zero counters. Reconcile element/member casualties only from a
    completed spawned sample or a durable eliminated status.

- Fold is a projection transfer, never an outcome.
  - Refuse fold during contact, cargo/captive pickup/delivery interaction,
    player vehicle occupancy, or pending crew population/seating. A vehicle's
    terminal captured flag is authority for a dismounted crew-only root, not by
    itself a reason to deadlock folding.
  - Preflight and sample every active, crewless, and terminal-survivor root
    before changing any durable row or deleting any runtime handle. Apply the
    complete handoff only after all roots validate; a later-root failure must
    leave the whole operation physical and unchanged.
  - Do not mark a casualty, refund a resource, complete an objective, or apply a
    convoy outcome during fold. Return to virtual authority at the sampled
    cursor and let rematerialization realize only durable survivors.
  - When a physical survivor count decreases, retire deterministic member slots
    by their exact slot-to-entity mappings and retain their casualty tombstones.
    Never infer which member died from a lower aggregate count or retire a seat
    suffix: seat zero may die while later seats and their frozen prefabs survive.
    Never increase living slots from a later runtime count or the original plan.
    One process-local bijection owns each frozen living slot: mission/group/slot
    identity maps to exactly one extant entity, and no entity may satisfy two
    slots. Readiness and casualty sampling must verify native/editable group
    membership. Only an explicitly mapped entity reporting `DEAD` or `DESTROYED`
    may create a new casualty; a missing/deleted handle is ambiguous and fails
    closed.
  - A player-bound or otherwise cleanly detached picked-up/delivered asset no
    longer owns the convoy projection and must not pin unrelated crew/vehicle
    roots physical across a delivery trip. Fold blocks only an ambiguous or
    still-exact-carrier attachment claim.
  - Re-run zero-survivor classification after the physical survivor refresh. A
    root whose last crew member dies during fold becomes `ABANDONED`/nonmobile,
    while a sampled damage fraction at the destruction threshold atomically
    marks the vehicle asset and element `DESTROYED`. A terminal crew handle that
    disappears during that refresh is authoritative zero, not permission to
    reuse the element's stale survivor count.

- Arrival and settlement have single owners.
  - Virtual arrival is the persisted route cursor reaching the route end.
    Physical arrival requires two samples from distinct campaign seconds within
    50 m of the actual endpoint, not merely a projected cursor. When a frozen
    cargo/captive slot exists, its assigned vehicle-zero carrier must still be
    active, mobile, crewed, and within that radius. Either path marks the active
    convoy mission failed through its normal objective/outcome flow; the legacy
    travel timer must not independently arrive a contract-version-1 mission.
  - Durable zero living crew completes the crew objective and lets the existing
    convoy-outcome service apply capture/delivery/reward consequences once. If
    cargo, captives, or recoverable vehicles remain unresolved, move the open
    operation to an on-station recovery hold at the durable asset anchor; allow
    that projection to fold/rematerialize without resurrecting crew, and do not
    settle merely because the combat objective completed.
  - If every carrier vehicle is destroyed/captured but its frozen cargo remains
    unresolved, that terminal ground-cargo root alone is enough to enter
    materialization. Do not require an intact abandoned vehicle or loop it back
    to invisible virtual state when ground projection fails.
  - Only after the required mission-specific recovery outcome or another real
    terminal mission result may the operation write one settlement ID, terminal
    result, settled duty, and retired held roster. Replays are no-ops.
  - Capturing a physical exact convoy vehicle is a cross-service handoff. The
    garage reservation must succeed, PhysicalWar must validate the frozen root
    and zero living crew, unregister the one authoritative vehicle handle, and
    return that world entity for deletion. Do not create a garage copy while the
    tracked physical vehicle remains usable. Apply the same handoff to a partial
    `ABANDONED` root; do not require every other convoy crew to be dead first.
    Before deleting the carrier, detach and unregister any unresolved frozen
    cargo/captive projection so a seated captive or child prop cannot be deleted
    with the vehicle. Recreate it from the durable terminal ground position.
  - Arrival evidence and the arrival outcome receipt are separate save
    boundaries. A restart after route arrival but before the outcome applies
    must restore the operation open; only the persisted outcome flag permits
    the `arrived` settlement. Likewise, generic objective completion must defer
    while an exact recovery outcome is pending—including supply cargo after crew
    elimination—or the coordinator will erase
    the on-station recovery hold one tick after crew elimination.

- Restore must converge to one virtual owner.
  - Validate one reciprocal mission/operation/manifest/held-batch/three-element
    aggregate before normalization. Conflicting or incomplete schema-52 links
    fail closed; they do not fall back to contract version `0`.
  - Exact member-slot tombstones plus each convoy element own survivor truth.
    Active-group survivor counters are derived physical snapshots: after the
    reciprocal aggregate validates uniquely, rebind those counters from the
    element instead of quarantining a valid save merely because the snapshot
    lagged at capture time.
  - Validate disposition against the linked vehicle facts: active/abandoned/
    arrived roots are unresolved, destroyed roots require destruction evidence,
    captured roots require capture/delivery evidence, and retired roots cannot
    remain inside an open operation. A mismatch is corrupt authority, not a
    recoverable projection delay.
  - In an open held batch, casualty tombstones are legal only for member slots.
    Group, vehicle, and cargo roots remain noncasualty registered authority until
    terminal settlement. Enumerate legal open duty/resume/materialization/position
    pairs; an open `RETIRED` duty is corrupt because it can neither advance its
    route nor rematerialize after normalization.
  - Clear process-local vehicle/group/native handles, physical flags, arrival
    samples, and unresolved convoy-cargo spawned/runtime flags. Retain generated-
    route progress, element positions/vehicle state, cargo assignment, stable
    slot identity, and confirmed crew casualties, then resume one held virtual
    projection. MissionRuntime must actually recreate cargo before readiness can
    turn the operation physical.
  - A settled receipt may be restored while the normal mission row is still
    `ACTIVE`. Keep exact runtime/salvage protected until the mission service
    commits its success/failure status; cleanup must not infer salvage policy
    from the settlement receipt alone.
  - Settlement may retire still-living held member slots before physical cleanup.
    The retirement-only roster handoff may accept those settled noncasualty slots
    only with unchanged survivor counts and the same exact mapped entity/prefab;
    fold and capture must continue to require registered live authority.
  - A quarantined contract-version `-52` convoy must bypass generic mission-
    convoy asset creation, payload repositioning, and legacy runtime repair
    until normal failed-mission cleanup commits. Quarantine preserves the
    rejected graph as evidence; it is not a legacy fallback mode.
  - Missionless or partially unlinked exact-looking group rows are quarantine
    evidence too. Deterministic convoy prefixes, element IDs, and exact authority
    ID prefixes must make generic cleanup discard process handles only; it must
    never delete the durable orphan merely because no canonical mission can be
    resolved.
  - Settled exact groups, elements, assets, member slots, and receipts remain
    durable authority records after their process handles retire. Runtime
    cleanup must clear stale spawned/physical flags once and then become a true
    no-op; it must not increment revisions or dirty the campaign every tick.
    Generic spawn, survivor polling, capture pressure, threat, undercover,
    clear-area, spawn-placement, and UI queries must classify settled and `-52`
    roots as non-operational without zeroing or deleting their durable counts.
    Open crewless recovery vehicles remain operational assets but are not combat
    presence unless a living exact crew element still exists.

- Persistence is an authority boundary, not a passive byte copy.
  - Autosave, major-change, manual-checkpoint, and campaign-debug baseline capture
    must synchronously reconcile every physical exact member mapping before
    serialization. This closes the window where a newly dead mapped soldier could
    be saved as alive before the normal one-second physical-war tick.
  - If an outbound publication transaction is open, a mapped member/root is
    missing or conflicting, or a nonphysical operation retains member mappings,
    defer the checkpoint without flushing an older tracked snapshot or requesting
    an engine save point. Retain checkpoint intent and retry on the configured
    debounce; confirmed casualty retirement is monotonic and safe to capture.

- Publish one aggregate marker from operation authority.
  - Use the strategic cursor while virtual and the live authoritative position
    while physical. Suppress the three per-vehicle markers so map state does not
    imply four independent convoy lifecycles.
  - Keep the same aggregate current-marker identity during recovery. A pending
    cargo/captive may change its label and delivery destination, but exact ammo/
    armored recovery must not fall back to legacy per-vehicle markers and exact
    cargo recovery must not replace the aggregate with a separate outcome ID.
    Keep one aggregate destination marker for both cargo and vehicle recovery;
    aggregate-only means no per-vehicle IDs, not no destination.
  - Remove the aggregate marker after terminal settlement. Source compilation
    and deterministic state checks are not proof of client rendering or cleanup.

- The final stamped schema-52 tree identifies implementation
  `fa5e7e45dbd8741269e614e60c51d4edee6bf223`, passes repository validation, and
  passes a clean headless Workbench Game-module compile/create gate at 5,753 files/
  11,537 classes with CRC `e868739b`. A normal WorldEditor open created the same
  Game module and remained responsive for all 10 bounded samples without a
  script-error or native-crash signature. This is source/startup evidence only.
  Packaged proof remains
  open for three vehicles and seated drivers,
  movement, physical interception, fold/rematerialization, casualty persistence,
  arrival/outcome settlement, marker rendering/cleanup, and process restart.
  `HST_MissionConvoyOperationProofService` contributes nine deterministic
  `mission_convoy.*` assertions for admission/rollback, projection/fold gating,
  casualty restore, idempotent settlement, open/settled/recovery restore,
  aggregate-marker cleanup, and the materialization watchdog. Its admission/
  corruption subfixtures reject invalid cargo, foreign authority, invalid seat
  topology, forged arrival receipts, illegal lifecycle pairs, and casualty
  authority on non-member roots while preserving missionless exact-looking
  durable claimants. They are source fixtures rather than packaged behavior
  proof.
- Schema 52 also repeated the native zero-diagnostic heap failure before
  `Module: Game` while save validation and corruption proof work were still too
  monolithic. Extracting save validation and decomposing the proof into focused
  fixture methods restored the clean headless and normal-open gates without
  removing assertions; relocating a large body intact is not a sufficient fix.
- Packaged schema-50 through schema-58 certification remains independently open.

## Schema 53 Exact Enemy-Patrol Authority

- Opt in by order type and contract version, never by an ID-shaped coincidence.
  - Only a newly queued `HST_ENEMY_ORDER_PATROL` receives contract version `1`
    and operation type `HST_OPERATION_TYPE_ENEMY_PATROL`. Every patrol restored
    from schema 52 or earlier remains contract version `0` with no invented
    source, route, roster, operation, debit, refund, batch, or group identity.
  - `RequiresOperation()` should keep every nonzero contract out of legacy
    execution, while commander dispatch separately selects exact patrol, exact
    defensive QRF, quarantine, unsupported, or legacy ownership from both type
    and version. Never let a positive unsupported or negative quarantined row
    fall through to the old patrol timer.
  - The later schema-54 garrison-patrol policy uses its own operation type and
    purchased-roster authority. It does not reuse the enemy commander's
    proactive attack-resource order merely because both use patrol waypoints.

- Admission is one atomic proactive-resource and force-authority transition.
  - Resolve a usable persisted generated route before the debit. Freeze one
    executable infantry group root and every ordered member slot, then link one
    route/order/operation/manifest/held-batch/active-group aggregate.
  - The patrol uses the proactive attack pool, not the defensive-QRF attack/
    support refund helper. A post-debit admission rejection applies one exact
    full proactive refund; a committed operation settles only the integer
    survivor fraction. Persist the receipt identity so replay cannot debit or
    refund twice. Fold and materialization never own refunds.
  - Failed preflight must leave no partial exact graph. A committed replay must
    return the existing aggregate rather than create another route, roster, or
    debit. Scan every primary and secondary order/operation/manifest/batch/group
    identity before admission. Rollback may remove only the exact object
    references inserted by that attempt; a colliding foreign row is evidence,
    never cleanup ownership. Revalidate the frozen route/hash on committed replay.

- Keep the generated-route cursor generic and explicit.
  - `HST_OperationRouteCursorService` owns ordered route positions, a route
    contract hash, waypoint index, lap count, leg sequence, loop start/completion
    seconds, bounded virtual advancement, position resolution, and live-position
    rebasing. Do not overload the direct-QRF cursor with patrol-only loop rules.
  - Outbound travel ends at route position zero. One closed on-station lap starts
    at position one, walks every ordered position, and increments the lap only
    when it wraps to zero. Return-to-origin is a distinct leg represented by
    waypoint index `-1`; do not count return as another patrol lap.
  - Freeze/validate the generated-route contract hash. If the durable route no
    longer matches, quarantine the current-schema aggregate rather than silently
    accepting a different path.

- Physicalization is a projection transfer of the same roster and cursor.
  - Reuse the exact one-root infantry SpawnQueue/adapter. Release only durable
    living slots, preserve slot/entity bijection, and retire a casualty only from
    a mapped entity's authoritative dead state. Missing handles or aggregate
    count changes are not enough to choose a dead member. A deleted handed-off
    entity with no observed death state remains an unresolved binding: retain its
    adapter evidence and block movement, fold, settlement, save, and cleanup.
  - Before any live patrol movement, dematerialization, checkpoint, or campaign-
    stop settlement, prove one unique handed-off root and one unique live handle
    for every durable living member. Result/projection keys, slot ownership,
    entity IDs, entity handles, adapter cardinality, and PhysicalWar membership
    must all agree; cross-key or aliased claimants fail closed.
  - Fold samples the live position, rebases the current generated-route leg, and
    retains every confirmed casualty. Re-entry realizes only survivors at the
    durable cursor. Exclude exact patrol groups from legacy garrison-patrol,
    population repair, survivor, route, fold, and cleanup passes or both owners
    will mutate the same group.
  - Physical contact is an explicit hold. While contact evidence exists, keep the
    group physical, freeze the cursor, and consume the operation clock. When
    clear, restart the same frozen leg from the authoritative live position.
    Never apply delayed virtual catch-up across the contact interval.

- Restore and quarantine preserve evidence rather than guessing.
  - `HST_EnemyPatrolSaveValidationService` validates unique reciprocal order/
    operation/route/manifest/batch/group identity, manifest policy/hash, member-
    only casualty tombstones, legal route/lifecycle pairs, and a coherent
    proportional settlement receipt.
  - A coherent open restore clears process-local entity/native-group evidence and
    normalizes materializing/physical/folding state to one held strategic
    projection with the same waypoint/lap/leg cursor and casualties. Do not pass
    patrol rows through defensive-QRF restore normalization; its direct-route
    assumptions would corrupt the loop cursor.
  - Malformed current-schema patrol authority becomes contract version `-53`.
    Retain the rejected graph and failure evidence, hold/cancel process work, and
    prevent legacy execution. Migration records
    `migration_schema53_enemy_patrol_authority` while preserving historical rows
    at version `0`. Unsupported or type-corrupt rows linked to a patrol operation
    must enter this same typed quarantine before another exact or legacy owner can
    run them.
  - Quarantine cleanup is bounded, not permanent save denial. Defer capture while
    any adapter handle, PhysicalWar root/member, or process-local group binding
    remains. Once both runtime registries prove empty, normalize an open operation
    to strategic authority (or clear only process-local residue on a settled row)
    while retaining every durable diagnostic row and applying no refund.

- Persistence must close the physical observation race.
  - Before autosave, major-change capture, checkpoint, or campaign-debug baseline,
    reconcile mapped physical and dematerializing patrol casualties, prove the
    complete root/member/PhysicalWar bijection, and resolve an authoritative live
    position. If mapping, runtime ownership, or live position is missing or
    conflicting, defer capture before flushing an older tracked snapshot or
    requesting a savepoint. Retain save intent and retry on the bounded debounce.
  - Campaign-won/lost and setup settlement must run the same exhaustive patrol-
    scoped reconciliation before reading survivors. If it fails, leave the debit,
    settlement, runtime entities, and open patrol batch untouched; generic terminal
    cleanup must protect that patrol until authority becomes provable.
  - Current-schema restore keeps the generated-route cursor and casualty roster,
    but clears process-local physical handles/contact evidence. This is source
    normalization; a real process restart remains a separate packaged proof.

- Publish and prove from operation authority.
  - Publish one exact-patrol marker at the virtual cursor or live physical
    position, with durable living count and duty context. Remove it only after
    terminal cleanup. Marker model presence is not client widget-render proof.
  - `HST_EnemyPatrolOperationProofService` contributes ten focused
    `enemy_patrol.*` assertions: admission, replay/refund, route loop, projection/
    roster, contact hold, settlement, restore, corruption, dispatch isolation,
    and marker lifecycle. Keep proof methods focused; large monolithic proof or
    migration bodies can reproduce the native zero-diagnostic Workbench compiler
    failure seen in earlier schemas. These source fixtures now cover foreign-row
    collision preservation, corrupt-route replay rejection, physical-shaped
    restore normalization, reciprocal backlink quarantine, exact-mode orphan
    isolation, and same-target urgent-order priority. Native entity/waypoint/
    adapter-handle and real process-restart behavior remains packaged proof.
  - The final stamped schema-53 tree identifies implementation
    `8ab694fdf61e56c6a5e343782f2225660d3aeeb7` and has clean headless Game-module
    compile/create evidence at 5,757 files/11,550 classes with CRC `3232b15a`;
    a normal WorldEditor open remained alive for ten samples
    over 20 seconds without a crash signature. It still needs packaged movement, contact, fold/
    reprojection, accounting, marker rendering/cleanup, and process-restart proof.
    Schema 54 selects the next source target: newly issued purchased resistance
    garrison patrols.

## Schema 54 Exact Purchased-Garrison Patrol Authority

- Opt in by purchase policy, faction, and complete reciprocal graph.
  - Only a newly issued `garrison_exact_patrol_2` resistance quote and manifest
    may create `HST_OPERATION_TYPE_GARRISON_PATROL` contract version `1`.
    Historical `garrison_exact_all_or_nothing_1` purchases, initial-map
    garrisons, enemy garrisons, and other aggregate records stay on their legacy
    representation. Never infer opt-in from a garrison ID, accepted-manifest ID,
    zone owner, or a group that happens to have patrol waypoints.
  - The manifest freezes one executable group element whose prefab is a
    `NotSpawned` AIGroup container. It is an empty execution root, not an empty
    manifest: all arbitrary purchased member slots remain required, priced,
    hashed, ordered, and spawned separately. This supports a requested roster
    that need not equal an authored group prefab's built-in composition.
  - Admission links the accepted quote, manifest, committed money/HR receipts,
    stable garrison backlink, operation, held batch, active group, and generated
    route atomically. Policy-v2 exact members do not increment
    `m_iInfantryCount`; capacity reads the living held batch through the accepted
    manifest link. Missing/ambiguous open authority remains capacity-reserved
    until typed validation resolves it without guessing.
  - Every recruitment-capacity reader must use the same mixed-authority sum:
    legacy `m_iInfantryCount`, `CountExecutableManifestInfantry()` for exact
    living/reserved patrol slots, and the zone's active legacy infantry. This
    applies to reports, map-target eligibility, and legacy direct recruitment;
    omitting the exact term can overbook a zone without changing the exact
    operation itself.
  - A terminal operation frees exact capacity only when it reciprocally owns the
    same manifest, quote, faction, assignment zone, exact contract, and fixed
    settlement receipt. A coincident or foreign settled operation ID is not
    evidence that the linked roster retired; malformed or ambiguous authority
    keeps the full accepted count reserved until typed validation resolves it.
  - Legacy direct recruitment is all-or-nothing at its own mutation boundary.
    Reject a request that does not fully fit, then verify the exact infantry and
    vehicle deltas after `AddAbstractForces()` and before spending money or HR.
    If the delta is partial, roll it back before any charge. Do not prorate a
    caller-supplied bundle cost after silently admitting fewer units.

- Treat the operation as one indefinitely assigned local patrol.
  - Freeze a small generated route around the immutable assignment zone and use
    the shared ordered waypoint/lap/leg cursor. The operation starts on station,
    advances the same loop virtually outside the render bubble, and starts the
    next leg after every arrival. There is no required-lap terminal transition;
    wrap only the very large persisted lap counter, not the duty itself.
  - Materialization uses the existing in/out hysteresis. Release the empty root
    and only durable living member slots, obtain a full adapter-to-PhysicalWar
    root/member bijection, issue the persisted current route leg, and then move
    position authority to live state. Recent confirmed casualties hold route and
    fold long enough for authoritative reconciliation.
  - Dematerialization first reconciles each mapped casualty and validates every
    survivor binding, retires only runtime projection ownership, requeues the
    successful batch into strategic hold, folds the live position into the
    cursor, and resumes virtual movement. Never add survivors to the aggregate
    garrison count; the held exact slots already own them.

- Survivor-only rematerialization must validate against living authority.
  - The adapter's original roster size remains equal to the accepted manifest,
    but its current infantry count must equal `CountDurableLivingMemberSlots()`
    after the first successful handoff, or the strategic living count before
    that handoff. Comparing current infantry to the original accepted count
    resurrects casualties or rejects a correct partial roster.
  - A durable casualty needs a slot-mapped entity's observed dead state. A
    deleted or missing binding without observed death remains unresolved and
    blocks movement, fold, settlement, save, and cleanup; it is never selected as
    a convenient casualty.
  - Exact binding preflight must prove membership, not only cardinality. For
    every durable living slot, the mapped adapter entity must also be the entity
    registered under that active group in PhysicalWar. Equal counts can hide a
    crossed member; retiring the adapter entity in that shape can delete it
    before discovering the different PhysicalWar member still owned by the
    projection.

- Keep exact and legacy garrison ownership mutually exclusive.
  - Tag exact groups with `exact_garrison_patrol` and recognize their canonical
    operation, policy, projection, route, and composition backlinks. Exclude them
    from legacy zone activation composition, zone-garrison waypoint assignment,
    delayed population repair, abstract survivor fold, route fallback, and
    generic group cleanup. Conversely, do not let the exact service claim
    historical policy-v1, initial, enemy, vehicle, or unlinked aggregate groups.
  - The generic `UpdateRuntimeGroupSurvivors()` loop must skip exact enemy and
    exact garrison patrols before repair, aggregate-count mutation, elimination,
    or terminal cleanup. The adapter's mapped member-slot observation is their
    sole physical casualty authority; running both owners can remove registry
    evidence before the exact tombstone is committed.
  - Typed-quarantine groups remain durable diagnostics, not campaign actors.
    `HST_CampaignState.IsOperationalActiveGroup()` must reject them before its
    broad non-convoy fallback so they cannot exert capture pressure, reserve
    spawn-clearance space, enter combat queries, or reach generic lifecycle
    mutation after their physical projection is gone.
  - This is one infantry-root slice. Generic garrison vehicles, assets,
    emplacements, multiple roots, and conversion of historical aggregate
    defenders remain separate future contracts.

- Settlement is terminal retirement, never a refund or survivor transfer.
  - Owner change, all members dead, campaign stop, setup, or typed spawn/route
    failure settles the fixed `exact_garrison_patrol_terminal` identity once.
    Reconcile and retire any live
    projection first, retain the exact terminal survivor count for evidence,
    then remove the runtime batch/group/route and accepted-manifest garrison
    backlink only when every identity is unique.
  - Owner change is not permission to bypass live-authority reconciliation. If a
    `PHYSICAL` or `DEMATERIALIZING` operation still has durable living slots but
    its adapter and PhysicalWar runtime have vanished, quarantine the graph and
    preserve every row. Never settle the stale survivor count merely because the
    ownership transition is terminal; a missing runtime is still missing
    casualty evidence.
  - The purchase already committed money and HR. Owner change, casualty loss,
    fold, quarantine, and campaign stop return zero resources and add zero legacy
    infantry. A replay must find the same terminal result and settlement ID or
    quarantine the conflict.

- Schema-54 migration and quarantine preserve history conservatively.
  - `HST_GarrisonPatrolSaveValidationService` records
    `migration_schema54_exact_garrison_patrol` for a pre-54 restore and creates no
    exact operation, roster, batch, group, route, casualty, or settlement
    identity for historical purchases/aggregates. A pre-54 exact-shaped
    operation is unsupported evidence, not permission to execute it.
  - Current-schema validation requires one unique quote/manifest/garrison/
    operation/batch/group/route graph, policy/hash/slot integrity, legal
    materialization/position state, and coherent settlement. Normalize a valid
    open physical-shaped graph to one held virtual batch with the same cursor and
    casualties after clearing process-local handles.
  - Malformed current authority becomes version `-54` with no refund, legacy
    conversion, or guessed retirement. A quarantine may be raised after the
    adapter tick, so pre-save normalization must recognize `-54` independently
    of the current `+1` classifier. Defer capture until the linked batch is
    terminal-cancelled/final-failed with no registered, spawning, or cleanup
    slots and both adapter and PhysicalWar registries prove empty. Only then may
    process-local group residue be cleared while the diagnostic graph remains.
  - A physically handed-off batch is already `SUCCEEDED`, so generic queue
    cancellation must refuse it. Typed quarantine drainage must instead resolve
    one unambiguous projection/result runtime owner that also matches the
    quarantined operation ID plus another durable backlink, reconcile mapped deaths
    through a quarantine-specific slot callback, validate every remaining live
    binding, retire that exact runtime projection, and terminal-cancel its
    process-free slots directly. This terminalizer must not consume normal
    reprojection capacity and must work for a zero-survivor roster. Temporarily
    aligning a uniquely selected runtime group to its batch may enable existing
    adapter/PhysicalWar checks, but restore the corrupt diagnostic backlinks
    afterward. If any runtime key or binding is ambiguous, keep the projection
    quarantined and checkpoint blocked; never discard a possible casualty or a
    competing operation's entity.
  - Restore has a special proven-absence case: after restore normalization has
    confirmed or completed runtime retirement, normal strategic requeue can
    still fail because the nonterminal queue is full. Terminalize that already
    process-free successful batch at the failure site before applying `-54`;
    do not make typed cleanup mistake its living durable slots for unexplained
    missing runtime. Restore/save claimant scans must likewise require operation
    identity plus another backlink before tagging or cancelling a group/batch;
    one coincident projection ID is never destructive ownership.

- Persist and project only authoritative state.
  - Defer a real checkpoint while an exact infantry patrol is still
    `MATERIALIZING`. Staged roots/members have not completed the all-required
    handoff, so their process handles are not yet a complete durable roster and
    a mid-wave snapshot could otherwise resurrect an unrecorded death on
    restart. Resume capture after the batch becomes fully physical, returns to
    strategic hold, or reaches a typed terminal result.
  - Before every real capture and campaign-stop/setup settlement, reconcile
    physical or dematerializing mapped members, validate the complete binding
    graph, and sample the live position. A failure defers capture before stale
    tracked state is flushed and protects the batch from generic terminal
    cleanup.
  - Publish one exact patrol marker from virtual/live operation position with
    location, current owner, role, and durable survivors. Forces UI may report
    exact patrol infantry separately from legacy aggregate infantry, but neither
    presentation is roster authority.
  - Nine deterministic `garrison_patrol.*` assertions cover admission, replay/
    rollback, roster projection, route loop, projection/casualty hold,
    settlement, restore, corruption quarantine, and marker lifecycle.
    Deterministic fixtures are source evidence. The stamped Schema-54 tree
    identifies implementation `09a1470a4c27dbef866e8cbdba182a7df65fa027` and
    passes clean Workbench Game compile/create at 5,760 files/11,560 classes with
    CRC `c62de929` plus a ten-sample/20-second normal WorldEditor open. These
    remain source/Workbench gates. Packaged native
    waypoint movement, authoritative casualty
    observation, fold/rematerialization, save/restart, marker rendering,
    replication, reconnect, and JIP behavior remain open proof obligations.

## Schema 55 Exact Officer-Mission Guard Authority

- Opt in only from a newly started officer-assassination mission.
  - Only guard infantry created while starting `assassinate_officer` may receive
    `HST_OPERATION_TYPE_MISSION_GUARD` at contract version `1` and manifest policy
    `exact_assassinate_officer_guard_v1`. Historical officer missions,
    `assassinate_traitor`, `assassinate_specops`, and every other mission family
    stay contract version `0`. Never infer opt-in from a mission ID, an HVT
    runtime row, a nearby hostile group, or an exact-looking projection ID.
  - Freeze one catalog-backed `NotSpawned` infantry execution root and every
    ordered guard member slot. The root is an empty container; it must not
    self-populate. The manifest contains no vehicle, projected asset, money, HR,
    equipment, attack-resource, or support-resource authority.
  - Keep the HVT outside every guard-force identity. It remains the mission
    objective and mission-runtime asset, never a manifest member, manifest asset,
    operation asset, group member, or reciprocal runtime backlink. A complete
    exact guard graph links only mission, operation, manifest, held SpawnQueue
    batch, and mission-owned active group.
  - Admission is fail-closed and atomic. Prepare exact authority before creating
    mission runtime, then admit it only after the separate HVT objective/runtime
    asset exists. If preparation, HVT creation, or reciprocal admission fails,
    roll back the uncommitted guard aggregate and fail the mission start with a
    typed reason; do not leave a detached batch or silently start legacy guards.

- Treat the guard as a route-less on-station survivor roster.
  - Derive a deterministic guard anchor 6-30 meters from the HVT so the group
    root and members do not overlap the target. Preserve the HVT position as the
    tactical target, but do not create a generated route, waypoint cursor, lap,
    travel ETA, or strategic movement owner for this operation.
  - Outside the materialization bubble, retain the held roster and its last
    authoritative position. There is no virtual guard combat or guessed passage
    of a fight. Only observed mapped physical deaths may retire member slots.
  - Inside the materialization radius, release the empty root and exactly the
    durable living member slots. After complete adapter-to-PhysicalWar handoff,
    live mapped members own position and casualty evidence. Contact and recent
    confirmed casualties hold the physical projection.
  - Outside the larger fold radius, first reconcile all mapped members and sample
    live position while the operation is `PHYSICAL` or `DEMATERIALIZING`; then
    retire only process-local projection ownership, return the successful batch
    to strategic hold, and preserve the same living/dead slots. Re-entry must
    realize only survivors. Missing runtime without observed death is unresolved,
    never a guessed casualty.
  - Assignment restart may require a fully `PHYSICAL` projection, but live-
    position resolution during fold/persistence must also accept the deliberate
    `DEMATERIALIZING` transition. Otherwise the service can change state before
    asking PhysicalWar for the position and then falsely quarantine a valid fold.

- Keep exact mission-guard ownership separate from every generic mission path.
  - Tag the active group with exact mode `exact_mission_guard`, force kind
    `mission_guard`, and intent `assassinate_officer_guard`. Require reciprocal
    mission/operation/manifest/batch/group ownership for mutating runtime access.
  - Non-mutating isolation scans should recognize both strong reciprocal claims
    and intrinsic exact/quarantine mode or contract evidence. An orphaned exact-
    shaped row must remain preserved evidence; it must not fall into legacy
    mission guard creation, survivor repair, route assignment, garrison fold,
    captured-zone cleanup, async population callbacks, or generic terminal
    deletion.
  - `HST_MissionRuntimeService` skips legacy hostile-group creation only for the
    current exact or quarantined officer mission. Historical officer missions and
    every other family retain their prior contract-zero path.
  - `HST_CampaignState.IsOperationalActiveGroup()` must reject quarantined guard
    claimants. A diagnostic group must not affect capture, combat, spawn
    clearance, or generic lifecycle mutation after runtime retirement.

- Keep guard outcome and HVT outcome independent.
  - When the last durable guard member dies, settle the operation `DESTROYED`
    with zero survivors. The `assassinate_officer` mission and its HVT objective
    remain active and playable; guard elimination is not mission success.
  - HVT mission success settles surviving guards `COMPLETED`. Mission failure or
    expiry, campaign stop, or setup settles them `CANCELLED`. Target-owner change
    settles `INVALIDATED`. A coherent spawn or assignment failure settles
    `SPAWN_FAILED`.
  - Freeze the terminal survivor count before terminalizing the queue. Queue
    cancellation can clear living slot flags, so reading survivors afterward
    loses settlement evidence.
  - Terminalize the typed queue batch before removing it from the aggregate.
    Final settlement cleanup must refuse a nonterminal batch instead of erasing
    pending, ready, or successful runtime authority prematurely.
  - Record the fixed settlement kind `exact_mission_guard_terminal` once. Every
    terminal result refunds zero resources and transfers zero survivors to a
    legacy garrison or mission group. Replay must reproduce the same operation,
    result, survivor count, and settlement identity or fail closed.

- Migrate and restore conservatively.
  - `HST_AssassinationGuardSaveValidationService` records
    `migration_schema55_exact_mission_guard` for a pre-55 restore and invents no
    manifest, root, member, casualty, operation, batch, group, projection, or
    settlement from historical officer missions, HVTs, legacy `mission_group_*`
    rows, or aggregate counts.
  - Current-schema validation requires deterministic unique mission/operation/
    manifest/batch/group identity; exact empty-root/member bijection; offset
    stationary assignment; zero route/vehicle/asset/resource authority; legal
    materialization/position pairs; separate HVT ownership; and the fixed
    terminal receipt.
  - Normalize a coherent open physical-shaped graph to one held virtual survivor
    roster after clearing process-local IDs. Defer a real capture while guard
    materialization is incomplete or a physical/dematerializing binding and live
    position cannot be proven.
  - A compact settled graph may legitimately omit its batch, active group, and
    terminal HVT runtime row. A settled `DESTROYED` guard may coexist with an
    active HVT mission. `COMPLETED`, `CANCELLED`, `INVALIDATED`, and
    `SPAWN_FAILED` require the corresponding non-active mission outcome.

- Quarantine malformed current authority without taking over the HVT.
  - Use contract version `-55` for malformed current exact authority. Preserve
    diagnostic rows and never fall back to legacy guard creation, guess a death,
    create an HVT backlink, issue a refund, or mark the HVT mission failed merely
    because guard authority is unavailable.
  - Strongly owned live projections may be drained only through typed mapped-
    casualty reconciliation and exact projection-key cleanup. Foreign,
    coincident, crossed, or ambiguous rows remain untouched and keep persistence
    deferred while possible runtime casualty evidence exists.
  - A restored quarantined `SUCCEEDED` batch may be terminalized directly only
    after restore normalization proves both adapter and PhysicalWar registries
    are empty. Ordinary in-process disappearance is not that proven-absence case
    and must remain strict.

- Project status through the existing HVT presentation.
  - Append roster-authoritative `guards N`, `guards neutralized`, or `guard
    authority unavailable` to the existing HVT map marker and mission UI row. Do
    not publish a second operation marker, and do not treat presentation text as
    roster or outcome authority.
  - Focused `mission_guard.*` source assertions cover admission/legacy isolation,
    projection/HVT separation, typed settlement, restore/migration, corruption
    quarantine, and marker status. They do not prove native entity creation,
    actual adapter handles/casualties, physical return-to-anchor behavior, real
    save/restart, rendered UI, owner-change, campaign-setup, packaged networking,
    reconnect, or JIP.
  - The stamped Schema-55 tree identifies implementation
    `552c2c4ff5ac7608fa248c614480a254769b61a4`. The full foundation gate passes;
    Workbench Game validation loads 5,763 files/11,570 classes with CRC
    `0ec8950e`, and a normal WorldEditor open remains alive for ten samples over
    20 seconds without a crash signature. This is source/Workbench evidence;
    never promote it or the source fixtures into packaged runtime certification.

## Schema 56 Exact Traitor-Mission Guard Authority

- Schema 56 is the preceding stamped baseline. Its implementation is
  `bab5748d817ba434dae701cfbb3b92805d463678`, stamp
  `03a65cd33bee69c6320389803cdd5a2ec8576fb0`, and label
  `schema56-exact-traitor-guard`.

- Extend exact guard authority through an explicit mission-kind/version cutover.
  - Only guard infantry created while starting a new `assassinate_traitor`
    mission may receive `HST_OPERATION_TYPE_MISSION_GUARD` at contract version
    `2` and manifest policy `exact_assassinate_traitor_guard_v1`.
  - Schema-55 `assassinate_officer` guards remain exact at contract version `1`
    with their existing `-55` quarantine boundary. Historical or pre-56 traitor
    missions, `assassinate_specops`, and every other mission family remain
    contract version `0`; migration must not reinterpret them as exact guards.
  - Dispatch on mission kind plus contract version. Do not infer the traitor
    cutover from an HVT row, a mission ID, a nearby group, or exact-looking
    projection identifiers. Malformed current traitor authority uses quarantine
    version `-56` and must not fall through to officer or legacy handling.

- Reuse the exact route-less survivor contract without merging HVT authority.
  - Freeze one catalog-backed `NotSpawned` empty execution root and its ordered
    infantry members. It owns no route, vehicle, projected asset, money, HR,
    equipment, attack resource, or support resource.
  - Keep the traitor HVT outside the manifest, operation assets, member roster,
    group membership, and reciprocal guard-runtime backlinks. It remains only
    the mission objective and mission-runtime asset.
  - Preserve the deterministic on-station guard anchor offset from the separate
    HVT and retain the HVT position only as the tactical target. Never invent a
    generated route, strategic travel cursor, or virtual combat simulation.
  - The held member slots are the sole guard-strength authority. Materialize only
    durable survivors, retire only casualties observed through exact mapped
    physical authority, fold only after complete reconciliation, and rematerialize
    the same survivor set. Missing runtime without observed death remains
    unresolved rather than becoming a guessed casualty.

- Keep guard settlement typed, zero-value, and independent from the HVT outcome.
  - All guard members dead may settle the exact guard operation `DESTROYED` while
    the `assassinate_traitor` mission and HVT objective remain active. Guard
    elimination is not mission completion.
  - HVT success, mission failure/expiry, campaign stop/setup, owner change, and
    coherent spawn/assignment failure use the same typed guard terminal mapping
    established by Schema 55. Terminalize the owned queue batch before removing
    it, freeze survivor count before queue cancellation, and record the fixed
    exact guard terminal receipt once.
  - Every traitor-guard terminal path refunds zero resources and transfers zero
    survivors into legacy force state. Replay must match the existing operation,
    result, survivor count, and receipt or fail closed.

- Preserve pre-56 history and quarantine malformed current graphs.
  - Pre-56 validation records `migration_schema56_exact_traitor_guard` and creates
    no operation, manifest, root, member, casualty, batch, group, projection, or
    settlement authority for historical traitor missions.
  - Current-schema conflict handling records
    `normalization_schema56_exact_traitor_guard_conflict`, retains diagnostic
    evidence at contract version `-56`, and never guesses casualties, refunds,
    creates an HVT backlink, starts legacy guards, or fails an otherwise playable
    HVT mission solely because guard authority is unavailable.
  - Current exact validation requires the deterministic reciprocal mission/
    operation/manifest/batch/group graph, empty-root/member bijection, route-less
    stationary assignment, separate HVT ownership, legal projection pairs, and
    typed terminal receipt. A compact settled graph may omit the batch, group,
    and terminal HVT runtime row; a settled all-dead guard may coexist with its
    still-active HVT mission.

- Reuse the existing HVT presentation and keep proof claims bounded.
  - Append `guards N`, `guards neutralized`, or `guard authority unavailable` to
    the existing traitor-HVT marker and mission UI row. Do not publish a second
    guard-operation marker, and do not treat that text as roster authority.
  - Six focused source-proof categories cover admission/isolation, projection
    lifecycle, typed settlement, restore/migration, corruption quarantine, and
    marker status. These fixtures do not prove native entities, actual adapter
    bindings or casualties, real save/restart, rendered UI, owner change,
    campaign setup, packaged networking, reconnect, or JIP.
  - The stamped Schema-56 tree identifies implementation
    `bab5748d817ba434dae701cfbb3b92805d463678` and build label
    `schema56-exact-traitor-guard`. The full foundation gate passes. Workbench
    Game validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
    reported `Script validation successful`. Its bounded hidden normal
    WorldEditor open stayed alive for all ten samples over 20 seconds, and the
    latest log had no script-error/crash signature. These are source/Workbench
    gates only; the native/package obligations above remain open.

## Schema 57 Exact Spec-Ops-Mission Guard Authority

- At the Schema-57 historical checkpoint, campaign persistence schema was `57`.
  Its stamped implementation is
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
  `schema57-exact-specops-guard`. The full foundation gate passes, including
  Schema-55/56/57. Stamped Workbench Game validation loaded 5,765 files/11,576
  classes with CRC `e0b8578e` and `Script validation successful`. The bounded
  hidden normal WorldEditor open stayed alive for 10/10 samples over 20 seconds,
  and its log had no script-error/crash signature. These remain source/Workbench
  gates rather than packaged behavior proof.

- Opt in only guards created while starting a new `assassinate_specops` mission.
  Use `HST_OPERATION_TYPE_MISSION_GUARD` contract `3`, policy
  `exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
  quarantine `-57`. Officer `1`/`-55` and traitor `2`/`-56` remain unchanged.
  Historical/pre-57 spec-ops and unsupported families remain contract `0`;
  ordinary `mission_group_*` rows are not exact claimants.

- Generic spec-ops composition may propose multiple groups. Contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster beneath
  the exact empty root. Discarded candidate groups must not create mission,
  operation, manifest, batch, active-group, or persistence authority.

- Reuse the exact route-less empty-root/member roster and keep the HVT outside
  every guard-force backlink. Durable slots alone own guard strength through
  materialization, mapped casualties, fold, restore, and survivor-only re-entry.
  Do not create a route, virtual-combat clock, vehicle, projected asset, or
  resource authority. Missing runtime without observed death remains unresolved.

- Preserve typed HVT-independent settlement. All guards dead may settle
  `DESTROYED` while the HVT mission remains active; the existing mission/owner/
  spawn mappings still record one `exact_mission_guard_terminal` receipt with
  zero refund and no legacy-force transfer. Status remains on the existing HVT
  marker/UI row rather than a duplicate operation marker.

- Pre-57 restore records `migration_schema57_exact_specops_guard` without
  inventing authority. Malformed current graphs record
  `normalization_schema57_exact_specops_guard_conflict` and retain diagnostic
  evidence under `-57` without fallback, guessed casualty, HVT backlink/failure,
  refund, or force transfer. Coherent physical rows normalize to held survivors;
  compact settled rows retain the same accepted omissions as prior guard families.

- Six `specops_guard.*` source-proof categories cover admission/family isolation,
  survivor projection/HVT separation, typed zero-refund settlement, restore/
  migration, corruption quarantine, and marker status. Native entities, adapter
  casualties, actual save/restart, rendered UI, owner change, campaign setup,
  packaged networking, reconnect, and JIP remain open.

- Schema 57 exhausts the assassination-guard family. Schema 58 is its separate
  successor cutover for newly started `rescue_pows` only; Schema 59 is the
  current radio-site lifecycle boundary.

## Schema 58 Exact POW-Rescue Authority

- Campaign persistence schema `58` is the preceding stamped source/
  Workbench baseline at implementation
  `f0ba07ff2bc295d12542a3ea34b4c913e99b1869` with build label
  `schema58-exact-rescue-pows`. This is the ninth explicit family consumer across
  seven operation types. Use a separate
  `HST_OPERATION_TYPE_MISSION_RESCUE`; do not extend the assassination-guard
  owner. Only newly started `rescue_pows` use contract `1`, policy
  `exact_rescue_pows_v1`, intent `rescue_pows_guard`, projection contract `1`,
  and quarantine `-58`. Historical POWs, `rescue_refugees`, generic captive
  rows, and ordinary `mission_group_*` rows remain contract `0`.

- Freeze one composite manifest but split execution authority explicitly.
  - The guard subgraph is one catalog-backed `NotSpawned` root plus its exact
    ordered member slots. SpawnQueue and the infantry adapter execute only those
    rows.
  - The asset subgraph is exactly three required captive slots. Set the batch's
    external-asset-authority flag and omit captive result rows from expected
    queue cardinality, work acquisition, retry, retirement, and binding checks.
    Treat that omission as a privileged exemption: one shared validator must
    accept the frozen/hash-valid exact policy, intent, force kind, guard root,
    zero-vehicle shape, and all three deterministic captive descriptors during
    queue admission, replay, durable-batch validation, and adapter execution.
    The adapter additionally requires one unique reciprocal mission-rescue
    contract-1 mission/operation/batch/group graph and exactly three matching
    durable captive rows before it ignores the asset descriptors.
  - The rescue service creates exactly three deterministic mission-asset rows
    and owns their lifecycle. Generic mission asset initialization, hostile-
    group creation, repair, cleanup, and legacy action handlers must skip exact
    and quarantined rescue claimants.
  - Do not append the generic adapter-mode token to an exact-rescue external-
    authority group. `m_sSpawnFallbackMode` is a durable rescue contract
    discriminator used by runtime and restore validation; changing it after
    root registration makes the otherwise valid root-to-member lifecycle
    quarantine itself.

- Treat captive state as durable typed authority, not compatibility booleans.
  - Persist ordinal; HELD/FREED/FOLLOWING/BOARDING/BOARDED/EXTRACTED/KILLED;
    stable escort identity; stable carrier ID; seat token; command request and
    result; casualty/extraction receipts; projection ID/generation; transition
    time; and revision.
  - Derive old picked-up/delivered/destroyed/attached/interaction fields from
    that typed state for existing UI only. Never use those projections to infer
    a typed transition on restore.
  - A missing `IEntity`, runtime row, or carrier projection is never evidence of
    death. KILLED requires an observed authoritative damage component in the
    destroyed state. The captive prefab's lifecycle component reports that
    evidence to the coordinator, which routes it through the rescue service.

- Keep player commands replay-safe at both presentation and domain boundaries.
  - Context actions must resolve the live mission: exact contract-1 rows expose
    only the next legal action, while quarantine is hidden and never falls
    through to legacy booleans.
  - Route visible commands with the stable player identity and command request
    ID. Keep a bounded typed ledger of accepted and rejected request/actor/command/result/
    revision rows and replay it before mutable distance, disposition, player-
    projection, or HQ checks; otherwise a successful HELD-to-FREED request can
    be misread as a later extraction request after state changes. Reject a
    reused request ID if its actor/command fingerprint changes or if another
    captive already owns it. Retain at most eight rejected domain rows; when
    necessary, evict only the oldest rejection so rejection spam cannot block a
    later legal transition. Never evict an accepted request identity. Stop
    admitting nonterminal accepted rows at `MAX - 1` so the last slot remains
    reserved for terminal extraction.
  - Free requires HELD and records FREED without claiming an escort. Follow
    requires FREED and records the stable escort. Extraction requires an
    existing custody state, player/captive proximity, and the frozen HQ target.

- Vehicle evidence must be durable and observed.
  - FOLLOWING discovers a carrier only from its connected stable escort's
    current occupied vehicle. Require a replication-backed/runtime-tracked ID;
    reject the position-derived `vehicle_local_*` fallback.
  - Register/update the durable runtime-vehicle row, transition to BOARDING,
    actuate native compartment entry, and advance to BOARDED only after same-
    carrier seat evidence. Persist the first seat token and rebind the saved
    carrier token on restore instead of recomputing identity from the new
    process entity.
  - A temporarily absent carrier never implies captive death. Outside expiry
    grace, a disconnected escort releases FOLLOWING/BOARDING/BOARDED authority
    back to FREED, clears carrier/seat identity, and lets a connected player
    explicitly claim the POW. During grace the escort set is frozen, so a
    disconnect invalidates custody and fails the grace path instead of silently
    transferring it.

- Render folding must preserve mission semantics.
  - HELD and FREED captives may delete their process projection outside the
    player/event bubble and respawn at their last authoritative position.
  - FOLLOWING, BOARDING, and BOARDED remain projected because they are bound to
    an escort/carrier. Terminal captives fold and retain their typed receipt.
  - The guard roster uses its exact stationary materialize/fold lifecycle. Do
    not simulate virtual guard combat or casualties, and do not let all guards
    dead settle the rescue mission.

- Outcomes remain owned by the normal mission pipeline.
  - Any one of the three casualty receipts yields the exact failure candidate.
    Exactly three unique HQ extraction receipts yield the success candidate.
    The coordinator calls the ordinary failure/completion path, so money, HR,
    aggression, town support, tasks, notifications, and settlement remain
    once-only and do not gain a parallel rescue reward owner.
  - Apply the configured support reward only to an applicable target town.
    Guard elimination alone changes only guard materialization state.
  - Before a failed/expired/cancelled rescue settles, release any surviving
    FOLLOWING/BOARDING/BOARDED row to FREED and clear escort/carrier/seat links.
    Settled history must not depend forever on a mutable runtime vehicle that
    can later be destroyed or removed for unrelated reasons.
  - When the base timer reaches zero, grant a single 300-second extraction grace
    only if all three living POWs are already FOLLOWING/BOARDING/BOARDED under
    stable escort identities. Grace forbids new free/follow claims. Death still
    fails, three extraction receipts still succeed, and the deadline must not
    produce both expiry and success. Derive the grace deadline from the frozen
    base deadline (`base + 300`), never from the late tick time, so a frame hitch
    cannot create a save that quarantines on restore.

- Replicate action legality as a small fail-closed DTO on each captive entity.
  The server publishes evaluated/exact-contract/disposition/active/quarantine/
  revision state; unchanged values do not bump replication. Clients and late
  joiners hide all captive actions while classification is pending, show only
  the single legal exact action after classification, and show none for inactive
  or quarantined authority. Server transition validation remains authoritative.

- Restore must fail closed without invention.
  - Apply rescue strong-claimant exclusions before generic migration regardless
    of the saved schema number. Otherwise a forged pre-58 exact-looking row can
    be generically backfilled before the Schema-58 validator quarantines it.
    Plain historical contract-zero rescue rows are not claimants and continue
    through legacy normalization.
  - Require a unique valid generated site on the target zone, exact site/target
    position, reciprocal graph identity, frozen manifest hash and slot bijection,
    legal typed transitions/receipts, and coherent grace/settlement. Clear only
    process bindings; preserve stable escort/carrier/seat and outcome evidence.
  - Record `migration_schema58_exact_rescue_pows` for pre-58 preservation and
    `normalization_schema58_exact_rescue_pows_conflict` for malformed strong
    claimants. Quarantine creates no fallback, captive, casualty, extraction,
    reward, refund, or force transfer.

- Current stamped source gates: the full foundation suite passes. Final stamped-
  tree Workbench Game validation loaded 5,770 files/11,594 classes with CRC
  `aa73883a` and reported
  `Script validation successful`. The bounded hidden normal WorldEditor open
  stayed responsive for 10/10 samples over 20 seconds with zero crash/error
  matches. These are source/Workbench gates only. Native captive/guard behavior,
  natural vehicle seating, actual save/restart, rendered marker/action state,
  owner change, campaign setup, packaged networking, reconnect, and JIP remain
  open until a fresh published runtime run.

## Schema 59 Durable Radio-Site Lifecycle

- Campaign persistence schema is now `59`. Every configured radio zone receives
  one contract-1 `HST_RadioSiteState` with deterministic site and target IDs.
  The row owns immutable authored prefab/position provenance separately from
  the current target prefab/position, BORROWED_WORLD or GENERATED_CAMPAIGN
  ownership, ONLINE/DESTROYED/REBUILDING lifecycle, one active lifecycle mission
  lock, typed last-transition request/from/to/revision, and destruction/rebuild
  receipts. The stable site target ID is durable identity, not a process handle;
  each mission projection uses its own deterministic runtime-entity ID so two
  mission histories cannot alias one physical row. Every mission asset snapshots
  the ownership and authored descriptor present at admission, so a later
  ownership handoff cannot relabel historical evidence. Quarantine uses `-59`.

- Initial projection binding is conservative. Search the logical zone within a
  bounded radius, normalize child hits to roots, accept supported transmitter
  prefabs, and require exactly one unclaimed candidate before freezing identity.
  Descriptor fallback is valid only when a concrete prefab identity is absent;
  light/base wrapper pieces must not become full transmitter candidates. Never
  create a generated fallback for an unresolved initial site. The supported
  authored transmitter exposes retained multiphase damage: its final damage
  phase does not delete the world object. Rebinding a frozen borrowed target
  therefore requires the exact prefab and a 0.75-meter match to its frozen
  position, rather than accepting another nearby transmitter. Physical
  projection/evidence may allow 12 meters because bounded safe-ground placement
  can move a spawned projection without changing its logical authored binding.

- Exact radio missions are lifecycle transitions, not generic destroy assets.
  `destroy_radio_tower` admits only a resolved ONLINE site and remains ONLINE
  until success physically destroys the bound target. Success records a fresh
  destruction receipt and moves DESTROYED; failure/expiry/campaign stop leaves
  ONLINE. `dynamic_stop_tower_rebuild` admits only DESTROYED with a destruction
  receipt, moves immediately to REBUILDING, and targets the dedicated
  construction-equipment prefab. Permit only one stop-rebuild admission for a
  given tower-destruction receipt/epoch. Destroying the equipment records the
  rebuild-attempt receipt and returns DESTROYED without replacing the tower's
  destruction receipt or advancing its epoch; a replay or second stop request
  in that epoch must fail closed. Failure/expiry/campaign stop records rebuild
  completion and produces one generated ONLINE replacement.

- Treat direct explosive or destroyed-event APIs as untrusted observations.
  Borrowed authored-target destruction requires the reciprocal active
  site/mission lock and revision, the tracked projection's authoritative damage
  state, and the frozen position. Generated transmitter or rebuild-equipment
  demolition additionally requires the live projection's matching
  `HST_MissionAssetComponent`, a bounded position, the frozen mission-time
  ownership/provenance snapshot, and a unique key in a persisted bounded
  evidence set. Replayed keys are already applied; exhaustion fails closed.
  When accepting a new physical evidence position, update the asset's current/
  last-known position and the one reciprocal runtime row's position together;
  destruction must also set both asset and runtime destroyed state together.
  Missing, duplicate, wrong-mission, wrong-kind, wrong-prefab, or already-
  divergent runtime rows must quarantine instead of creating a save-invalid
  position or destruction split.
  Raw damage without that evidence is repaired/reprojected without erasing its
  durable demolition progress. Check `SetHealthScaled` and the resulting damage
  state before committing destruction; likewise verify reset healing and exact
  rollback. A refused physical write quarantines or rejects rather than becoming
  a claimed durable outcome. A missing handle never proves destruction.

- A borrowed target may be absent because its world cell is not projected. Keep
  the reciprocal exact mission aggregate, clear all three mission/asset/runtime
  spawned flags, and use `radio_site_projection_pending`; if destruction was
  already proven, preserve `radio_site_target_destroyed`. Missing, duplicate, or
  cross-linked runtime rows quarantine. Do not let generic mission runtime, zone
  composition, objective ticking, commander progress, or generic failure
  settlement mutate exact/quarantined radio authority.

- Permanent generated ONLINE transmitters are lifecycle projections, not active
  mission witnesses. Keep verbose witness logging disabled, and gate the
  nearby-entity witness query on a complete configured asset/mission/role
  identity so an unbound permanent ONLINE replacement remains dormant instead
  of scanning the world every frame. An always-online site therefore does not
  run a continuous proximity/damage scan. Active mission evidence remains
  explicit through the lifecycle service.

- A new-campaign reset must settle the old physical ownership before swapping
  state. Reacquire the exact authored transmitter (including the frozen original
  behind a generated replacement), restore its health, verify it is no longer
  destroyed, and clear owned generated projections. If exact reacquisition or
  health restoration fails, restore the prior observable state and reject the
  reset rather than leaving the old world half-mutated.

- Apply exact radio outcome state before generic Strategic rewards/consequences,
  and reconcile projections before town income. Town radio influence must skip
  offline sites and choose the next eligible ONLINE site. Markers retain
  location plus owner and append lifecycle status; mission/asset UI describes
  the physical transmitter or construction-equipment requirement.

- Restore creates only logical ONLINE/unresolved rows for pre-59 saves. Keep
  terminal historical radio missions at contract `0`, fail active legacy exact
  IDs closed, and quarantine malformed strong claimants without inventing
  binding, damage, rebuild, receipt, or reward. Record
  `migration_schema59_radio_site_authority` and
  `normalization_schema59_radio_site_authority_conflict`. Generated ONLINE state
  is valid only with a prior destruction receipt and a completed-rebuild receipt.
  Quarantine closure must include site backlinks plus active, last-transition,
  destruction-receipt, and rebuild-receipt mission IDs. Fail/clean the corrupt
  current aggregate but preserve coherent already-terminal historical task and
  objective semantics.

- Six deterministic `radio_site.*` source assertions cover binding/admission
  isolation, lifecycle outcomes, receipt/revision replay and stale rejection,
  restore/migration/quarantine, influence suppression, and ownership/equipment
  handoff. The proof harness calls the production admission and outcome methods
  and the production bounded evidence helper, directly rejects a second rebuild
  admission in the same destruction epoch, proves linked quarantine cleanup,
  and substitutes only projection seams; it must not reimplement the durable
  transition graph. These assertions deliberately do not claim native authored
  discovery, natural explosives, damage-state reapplication, generated
  replacement, streaming re-entry, actual save/restart, rendered marker/UI
  behavior, campaign setup, packaged networking, reconnect, or JIP; all
  packaged verification gates remain open until they are executed.

## Native Reference Sources

- Native map config reference: `Configs/Map/MapFullscreen.conf`.
- Native map layout reference: `UI/layouts/Map/MapMenu.layout`.
