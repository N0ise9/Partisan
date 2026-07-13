# Partisan: Everon

Partisan: Everon is an original, persistent resistance campaign for Arma
Reforger. Players build a resistance movement on Everon while an occupying
force and an invading force pursue their own campaign objectives.

The project is in broad alpha. Its campaign foundation is substantial, but it
is still under active development and should be tested with disposable or
backed-up profile data. Source and Workbench validation do not by themselves
prove packaged server, persistence, multiplayer, or long-session behavior.

> [!WARNING]
> Fresh-start enemy commander authority and automatic legacy-profile migration
> have source and Workbench coverage but still require packaged-server and
> restart proof. Until those gates pass, use disposable or backed-up profile
> data for alpha testing.

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
  loadouts, garage vehicles, mission state, locations, and ownership
- Town support, population, security, capture, garrisons, recruitment,
  training, income, rewards, and penalties
- A mission catalog covering resistance operations, rescues, assassinations,
  convoys, defenses, logistics, and strategic objectives
- Resistance support requests, purchased forces, patrols, enemy responses, and
  selected exact virtual-to-physical operation lifecycles
- Abstract off-screen forces with render-bubble materialization, casualty
  reconciliation, survivor persistence, and map projection for supported force
  families
- Civilians, pedestrian danger behavior, civilian traffic, vehicle recovery,
  loot, build mode, radio sites, and strategic facilities
- An in-game command interface for setup, missions, forces, map/war
  information, arsenal, garage, members, and administration
- A one-button campaign diagnostic suite for controlled development testing

The implementation is not feature-complete. Several operation families remain
on compatibility paths, and many native, packaged, restart, reconnect, JIP,
multiplayer, and soak gates remain open. See the
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
| Script fallback campaign save | `$profile:Partisan/HST_CampaignSaveData.json` |
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

### Automatic Legacy Migration

Partisan uses `$profile:Partisan` as the only canonical generated-data root.
The entire retired profile tree is migrated automatically, including nested and
otherwise unrecognized files. Existing canonical files win and must not be
overwritten by older data; conflicting retired files are preserved in the
canonical legacy archive. Each new destination is copied through a byte-verified
staging file, rechecked before promotion, and verified again before its source is
removed. Directory-path conflicts are mirrored in the archive so the retired
tree can still be removed without discarding structure. The retired directory is
removed only after all files have been safely transferred or archived; a failed
or incomplete transfer leaves its source in place for a later retry.

Migration is guarded against re-entry within one running game process, but
Enforce file I/O does not provide an atomic no-overwrite promotion or an
exclusive cross-process file lock. Stop every server, client, and Workbench
instance that shares the profile before the first migration, and let exactly one
process complete startup before another process uses that profile.

This cleanup path is currently receiving fresh packaged-server and restart
verification. Back up long-lived profile data before the first migration on an
alpha build. See [Migrations](docs/MIGRATIONS.md) for schema-specific behavior
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

The one-button suite is a diagnostic harness, not automatic runtime
certification. World entities, player state, service caches, package identity,
and real process restarts sit outside the cloned campaign-state boundary. Use a
separately managed disposable profile for external, restart, package,
multiplayer, and soak profiles. Fresh startup, profile migration, save, restart,
and enemy-authority behavior remain external gates until a packaged run proves
them together.

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
3. Focused and Full Campaign Debug execution
4. Real profile save, process restart, reload, and reprojection
5. Packaged dedicated-server/client, multiplayer, reconnect, JIP, and soak proof

Do not promote a source-only or Workbench-only pass to runtime proof. When
testing a packaged build, capture the build identity, server/client logs, debug
artifacts, and save/restart evidence together. Close Workbench instances after
validation so stale processes do not retain resources or confuse later logs.

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
