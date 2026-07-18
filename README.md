# Partisan: Everon

Partisan: Everon is an original, persistent resistance campaign for Arma
Reforger. Players build a resistance movement on Everon while an occupying
force and an invading force pursue their own campaign objectives.

The project is in broad alpha. Its campaign foundation is substantial, but it
is still under active development. Campaign Schema 71 and runtime-settings
Schema 24 are the current persisted contracts.

> [!WARNING]
> Automated source, Workbench, focused engine, and scoped fresh-process checks
> do not yet close the broad packaged-server, full-campaign restart,
> multiplayer, or soak gates. Until those gates pass, use disposable or
> backed-up profile data for alpha testing.

## Campaign Overview

The Everon preset uses a three-way war:

- FIA: the player resistance
- US: the occupying force
- USSR: the invading force

The campaign is server-authoritative and combines persistent strategic state
with physical AI near players and active objectives. Its current broad-alpha
feature set includes:

- Setup-phase HQ selection, HQ relocation, Petros interactions, respawn, and
  campaign lifecycle controls
- Persistent campaign state, economy, resources, membership, arsenal,
  loadouts, garage vehicles, selected durable field-vehicle transforms and
  abstract cargo, mission state, locations, and ownership
- Town support, population, security, capture, garrisons, recruitment,
  training, income, rewards, and penalties
- A mission catalog covering resistance operations, rescues, assassinations,
  convoys, defenses, logistics, and strategic objectives
- Resistance support requests, purchased forces, patrols, enemy responses, and
  virtual-to-physical operation lifecycles for supported force types
- Abstract off-screen forces with render-bubble materialization, casualty
  reconciliation, survivor persistence, and map projection for supported force
  families
- Civilians, pedestrian danger behavior, civilian traffic, vehicle recovery,
  loot, build mode, radio sites, and strategic facilities
- An in-game command interface for setup, missions, forces, map/war
  information, arsenal, garage, members, and administration
- A one-button campaign diagnostic suite for controlled development testing

The implementation is not feature-complete, and several campaign systems still
need broader runtime and multiplayer validation. See the
[feature checklist](docs/FEATURE_CHECKLIST.md) and
[parity tracker](docs/PARITY.md) for the authoritative status.

## Requirements

- Arma Reforger
- The Partisan: Everon addon
- No hidden gameplay-addon dependency; the project depends on the Reforger
  base game only

Server and clients must use the same published addon version. A newer source
checkout does not update an already published or cached server package.

## Install and Run

### Workshop or Packaged Server

1. Install or subscribe to the current Partisan: Everon release.
2. Enable the addon on the server and every connecting client.
3. Select the Partisan: Everon campaign scenario.
4. Start with a new or backed-up profile while the project remains in alpha.
5. On first launch, review the generated settings before beginning a long
   campaign.

For a dedicated server, publish or package the current Workbench project before
testing it, then confirm that the server and clients report the same build
identity. Save/restart proof should use the exact package that produced the
save.

### Workbench

The repository provides two scenarios:

- `Missions/HST_Everon.conf`: full-island campaign scenario
- `Missions/HST_Dev.conf`: compact development and diagnostic scenario

Open the project in Arma Reforger Tools, compile the Game scripts, and run the
appropriate scenario. `HST_*` remains the internal source and resource naming
convention; the player-facing project name is Partisan: Everon.

## Controls and Command Interface

Press `I` in either Partisan scenario to open the alpha command interface.
During initial setup, complete HQ placement on the map first; the setup map owns
input until placement is complete.

- `MenuUp` / `MenuDown`: change selection
- `MenuSelect`: activate the selected command
- `MenuBack` or `I`: close the interface

Petros interactions open the same server-authoritative command path. Commands
that require a target use the in-game map and may require the player to carry a
map gadget. The server resolves the requesting player through network ownership
and validates costs, permissions, and campaign state before applying a command.

## Profile Data and Configuration

`$profile` means the active Reforger or dedicated-server profile root. Partisan
stores generated data under `$profile:Partisan`:

| Purpose | Path |
| --- | --- |
| Runtime settings | `$profile:Partisan/HST_Settings.json` |
| Campaign recovery journal, canonical slot | `$profile:Partisan/HST_CampaignSaveData.json` |
| Campaign recovery journal, recovery slot | `$profile:Partisan/HST_CampaignSaveData.recovery.json` |
| Loadout editor preferences | `$profile:Partisan/HST_LoadoutEditorSettings.json` |
| Personal loadouts | `$profile:Partisan/loadouts/v2` |
| Campaign Debug artifacts | `$profile:Partisan/debug` |

`HST_Settings.json` is generated on first load and contains JSON-safe `_comment`
fields explaining nearby options. Stop the server before editing it. The file
contains campaign, faction, economy, pacing, capture, world, civilian,
persistence, membership, and feature settings. Some values are defaults for new
campaign state and are not intended to rewrite facts already persisted in an
active campaign.

Notable feature switches include Game Master budget policy, infinite stamina,
and resistance support-group map tracking. Prefer the generated comments over
copying settings from an older schema by hand.

Campaign Schema 71 keeps a two-slot, crash-tolerant JSON recovery journal beside
native persistence. When native persistence is active, the exact staged campaign
snapshot advances the journal only after the native checkpoint completion
callback reports success; the two copies are therefore coordinated, not written
literally at the same instant. When native persistence is unavailable or the
session has explicitly entered degraded profile-only recovery, the journal
checkpoint is written synchronously.

Each journal generation stores the exact serialized snapshot payload, its
generation and parent identity, and an integrity fingerprint formatted as
`uuidv8-sha256-v1:<serialized-length>:<UUID>`. The writer changes only the
inactive slot, reads it back, and verifies the selected chain before the new
generation can supersede the prior known-good slot. This detects accidental
damage; it is not a cryptographic authentication or tamper-proofing mechanism.
The engine file API provides neither atomic rename nor an exclusive writer lock,
so only one server or Workbench process may write a profile at a time. Future
native authority is preserved and startup fails closed. Invalid journal data is
preserved during source selection, and a valid native save may still start; a
later verified native checkpoint may repair an ordinary invalid inactive slot.
Unsupported-future or ambiguous/conflicting history is write-fenced instead.
Without usable native authority, present invalid journal artifacts make startup
fail closed. Keep off-device backups for long-lived alpha campaigns because the
two slots share one profile and one storage device.

New native checkpoints use a version-2 native envelope that stores the exact
serialized payload string and fingerprints those bytes before parsing them on
load. Existing Schema-70 version-1 native rows are validated against their
reconstructed legacy layout and normalized before source comparison. Native and
journal snapshots are ordered by checkpoint sequence, then restore sequence,
then save second; equal order requires equal normalized fingerprints. An
administrative new-campaign reset retains the prior checkpoint/restore order,
exact-clones its prospective campaign, and commits that DTO to the verified
journal before old runtime cleanup or native staging. That JSON write-ahead
generation remains recoverable if native replication subsequently fails.

### Automatic Legacy Migration

Partisan uses `$profile:Partisan` as the only canonical generated-data root.
On first startup after upgrading, it automatically transfers the retired
profile tree, including nested and otherwise unrecognized files. Existing
canonical files take precedence, and conflicting older files are retained in a
legacy archive rather than overwritten. Campaign authority is stricter: a
difference between retired and canonical campaign files in either journal slot
is retained and startup fails until an operator resolves the conflict. The
retired directory is removed only after every file has been safely transferred
or archived; incomplete work is left in place for a later retry.

A valid raw Schema-70-or-earlier canonical campaign JSON is adopted as journal
generation zero. Its bytes remain untouched while the first Schema-71
checkpoint writes recovery generation one linked to that legacy authority.

Before the first migration, stop every server, client, and Workbench instance
that shares the profile, back up long-lived alpha data, and let one process
complete startup before another uses that profile. See
[Migrations](docs/MIGRATIONS.md) for schema-specific behavior, recovery details,
and compatibility rules.

## Administration

Add each administrator's raw 17-digit SteamID64 to
`membership.adminIdentityIds` in
`$profile:Partisan/HST_Settings.json`. Backend UUIDs, anti-cheat identifiers,
Workbench aliases, and per-session player IDs are not durable Partisan admin
identities.

After changing membership settings, restart the server and verify the runtime
admin-grant log before relying on the Admin tab. Admin actions include campaign
inspection, controlled state operations, checkpoint/reset helpers, focused
diagnostics, and the full campaign diagnostic suite.

## Full Campaign Debug

The Admin tab exposes `Run Full Campaign Debug`. The in-process suite is
restricted to `HST_Dev`, runs against a cloned campaign state, suspends normal
campaign checkpoints, and restores the live campaign state after completion or
cancel. Results are written under `$profile:Partisan/debug` as structured JSON,
summary text, and state-diff text.

The runner isolates bounded synthetic time from the shared campaign clock,
holds ordinary enemy-commander cadence while explicit production-path fixtures
own the clone, settles debug-created enemy orders through their typed operation
owners, and validates exact operation-marker backing through the marker
publisher. These protections reduce cross-case contamination; they do not turn
an incomplete run into certification.

The one-button suite is a diagnostic harness, not automatic runtime
certification. World entities, player state, service caches, package identity,
and real process restarts sit outside the cloned campaign-state boundary. Use a
separately managed disposable profile for restart, package, multiplayer, and
soak testing, and verify those behaviors in real packaged runs.

Detailed evidence and outstanding gates live in the
[Campaign Debug verification audit](docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md).

## Development and Validation

Run the repository foundation gate before Workbench or package testing:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate-foundation.ps1
git diff --check
```

The validation ladder is intentionally cumulative:

1. Foundation/static contract validation
2. Workbench Game-script compilation and target-configuration validation
3. Focused command-line engine autotests registered with the Game runtime
4. Focused and Full Campaign Debug execution in `HST_Dev`
5. Real profile save, process restart, reload, and reprojection
6. Packaged dedicated-server/client, multiplayer, reconnect, JIP, and soak proof

Registered focused engine autotests provide deterministic checks for selected
campaign-authority subsystems. They do not prove the `HST_Dev` coordinator,
Full Campaign Debug, world integration, persistence, restart, packaging, or
network behavior. Test identities and run evidence belong in the Campaign
Debug verification audit rather than this project overview.

The current implementation stamp is
`60596bf77d056b9e63ed1bbbf4d11c1941330fe6`, UTC
`2026-07-18T14:12:51Z`, label
`schema71-settings24-mixed-native-shutdown-restart`. Foundation validation
passes 874 references. Stamped Workbench validation loads 5,846 files and
11,899 classes at CRC `9a79a33a` with zero hard errors and zero owned cleanup
residue.
The focused authority testcase passes 1/1 with 41/41 exact conditions, an empty
failed list, and exact native-v1/native-v2/invalid-fingerprint/future-envelope
classification at 1/1/1/1.

Controlled shutdown is now an ordered native-authority fence. Read-only
readiness checks run before any one-way retention latch, state preparation is
followed by a second complete preflight before the first subsystem latch, and
the coordinator rejects new campaign ingress while draining. Retained loot,
rescue, field-vehicle, and active-force graphs are pinned, quiesced, and
revalidated on retry before persistence commits the shutdown checkpoint. This
checkpoint changes neither Campaign Schema 71 nor runtime-settings Schema 24.

The final strict five-process fresh-start proof passes periodic `AUTO`, typed
`MANUAL`, blocking `SHUTDOWN`, native no-save restore, and profile-fallback
no-save restore. The journal advances generations 1 -> 2 -> 3 and finishes with
canonical generation 3, two valid slots, and an exact parent chain; both restore
stages keep it read-only. Its field fixture reproduces the exact live-vehicle
and destruction-tombstone state through both recovery sources. All five
processes pass with every owned cleanup counter at zero. A separate three-stage
native-over-stale-journal proof passes 3/3, selects native authority, preserves
both journal files byte-for-byte with an exact chain, and also leaves cleanup at
zero. The administrative-reset stale-native proof also passes 3/3: it selects
the newer generation-3 JSON reset over deliberately stale native authority in a
read-only final process, preserves the exact two-slot chain and proof carrier,
rejects overlap without mutation, and leaves cleanup at zero.

The shutdown stage also proves one exact mixed-native rescue graph: a real
replicated carrier, three captives in `FOLLOWING`, seatless `BOARDING`, and
stable-seat `BOARDED` states, one two-member guard group, a rejected and cleaned
foreign occupant, and a real player occupant released before the final retry.
Native and profile-fallback restart both recover the old durable carrier key
onto the fresh replicated root, retain the exact seat token and logical
fingerprint, rematerialize the same guard survivors, and preserve live/durable
pose within the explicit restart tolerance. The five-process result is still a
scoped regression proof, not certification of every native shutdown topology or
adversarial branch.

A separate guarded three-process exact enemy-garrison-rebuild proof now crosses
the `delivery_pending` cut through the production JSON journal. Prepare freezes
the route at 225/300 meters with 9 accepted members, 8 living members, and one
confirmed casualty. Recover advances the final 75 meters and applies delivery
once: the exact survivor manifest remains held by the destination garrison,
aggregate infantry is not incremented a second time, and the original resource
debit receives one zero-refund settlement receipt. Replay is a semantic no-op.
The journal ends with canonical generation 1 and recovery generation 2; replay
changes neither slot. Every owned process, profile, guard, temporary, and spill
cleanup counter returns to zero.

The companion `physical_live_fold` cut starts from the same nine accepted slots,
eight living slots, one prior casualty, and `225/300m` route state. Production
authority crosses `VIRTUAL -> MATERIALIZING -> PHYSICAL/LIVE` with one native
root, nine adapter handles, and eight living runtime members. The observed root
moves 2.759m and closes 0.539m on its target before the production
`PHYSICAL -> DEMATERIALIZING -> VIRTUAL` fold. That fold retains the exact eight
survivors and one casualty with zero runtime residue; generation-1 persistence,
generation-2 recovery, and read-only/no-op replay all pass with cleanup zero.

Together these proofs close the exact rebuild fixture's `delivery_pending` and
`physical_live_fold` restart cuts, including its scoped native handoff, measured
movement, production fold, and survivor continuity. They do not prove natural
full-route travel or combat, other force families, unrelated mixed-native
mission graphs, multiplayer, JIP/reconnect, or soak. The focused
garrison-rebuild autotest also retains a
base-game reload/JUnit harness gap, so this checkpoint does not claim a fresh
focused-test result from that harness.

This proves the scoped fixture, not full fuel, partial-damage, attachment, or
physical-trunk parity. Arbitrary vehicle breadth, Workshop server/client,
migration breadth, markers, performance, and the broader runtime gates above
remain open.

Do not promote a narrower validation rung to broader runtime proof. When testing
a packaged build, capture the build identity, server/client logs, debug
artifacts, and save/restart evidence together. Close Workbench instances after
validation so stale processes do not retain resources or confuse later logs.
Accept a guarded run only after its owned processes, disposable profiles,
generated logs, guard roots, and temporary artifacts have all returned to zero.

## Project Documentation

- [Feature Checklist](docs/FEATURE_CHECKLIST.md): current feature status and
  next concrete work
- [Phase Plan](docs/PHASE_PLAN.md): dependency order, acceptance gates, and
  delivery roadmap
- [Parity](docs/PARITY.md): implemented behavior and remaining campaign gaps
- [Architecture](docs/ARCHITECTURE.md): service boundaries and authoritative
  state model
- [Migrations](docs/MIGRATIONS.md): campaign, settings, and profile migration
  contracts
- [Campaign Debug Verification Audit](docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md):
  runtime evidence and open verification gates
- [Enfusion/Enforce Notes](docs/HST_ENFUSION_ENFORCE_NOTES.md): engine mechanics
  and implementation lessons

## License and Notices

Partisan: Everon is licensed under the
[Arma Public License No Derivatives](LICENSE.md). See
[Third-party notices](THIRD_PARTY.md) for required attribution.
