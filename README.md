# Partisan: Everon

Partisan: Everon is an original, persistent resistance campaign for Arma
Reforger. Players build a resistance movement on Everon while an occupying
force and an invading force pursue their own campaign objectives.

The project is in broad alpha. Its campaign foundation is substantial, but it
is still under active development. Campaign Schema 70 and runtime-settings
Schema 24 are the current persisted contracts.

> [!WARNING]
> Automated source, Workbench, and focused engine checks do not yet close the
> packaged-server, restart, multiplayer, or soak gates. Until those gates pass,
> use disposable or backed-up profile data for alpha testing.

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
On first startup after upgrading, it automatically transfers the retired
profile tree, including nested and otherwise unrecognized files. Existing
canonical files take precedence, and conflicting older files are retained in a
legacy archive rather than overwritten. The retired directory is removed only
after every file has been safely transferred or archived; incomplete work is
left in place for a later retry.

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

Do not promote a narrower validation rung to broader runtime proof. When testing
a packaged build, capture the build identity, server/client logs, debug
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
