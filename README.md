# h-istasi

h-istasi is an original Arma Reforger implementation of the Antistasi
resistance-campaign concept. The first preset targets Everon with a three-way
war:

- FIA: player resistance
- RHS_USAF: occupying force
- RHS_AFRF: invading force

## Current Status

The repository contains the first engine-facing increment:

- Reforger and RHS project dependencies
- APL-ND licensing and third-party attribution
- Original Everon and compact development scenario shells
- Data contracts for presets, factions, maps, zones, balance, and missions
- A versioned campaign-state model
- Server-authoritative campaign, economy, mission, and native-checkpoint
  services
- A Community Edition 3.11.1 mission registry baseline
- FIA-owned development spawn anchors for authored hideout candidates
- HQ and Petros state hooks for the first playable resistance-campaign slice

This is a foundation build, not a public alpha. Physical Petros/cache/tent
entities, setup UI, spawned AI, persistence serializers, surveyed Everon
coordinates, and mission-specific world logic still need to be connected
incrementally.

## Requirements

- Arma Reforger 1.7.0.41 or newer
- RHS: Status Quo

## Design Rules

- Prefix addon-owned scripts and resources with `HST_`.
- Keep the strategic campaign server-authoritative.
- Use native Reforger persistence and session saves.
- Keep off-screen forces abstract and activate physical AI only around
  players and active objectives.
- Do not copy source assets from neighboring addons.
- Do not add hidden dependencies.

## Development Scenarios

- `Missions/HST_Everon.conf`: full-island campaign shell
- `Missions/HST_Dev.conf`: compact systems-development shell

See `docs/ARCHITECTURE.md` and `docs/PARITY.md` for the implementation map.
