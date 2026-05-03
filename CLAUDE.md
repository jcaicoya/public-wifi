# CLAUDE.md — Public Wi-Fi Cybershow

Use this file as the working context for resuming the app with Claude, Codex, or Gemini.

## Project Snapshot

Public Wi-Fi Cybershow is a Qt stage application for public Wi-Fi privacy storytelling. It has five runtime screens, a setup screen, live and demo operating profiles, a fake portal, a map view, a risk profile, and a scripted encryption analysis sequence.

The current app is already refactored to the Cybershow standard:

- Setup only appears in `--configure` or when no arguments are passed.
- `--show` and `--design` skip Setup and enter runtime directly.
- Runtime navigation uses `1-5`, arrows, and the bottom navigation bar.
- Letter shortcuts for primary navigation were removed.
- Demo mode is operator-controlled and does not auto-cycle screens.
- A pulsing non-interactive `DEMO` watermark appears only in demo mode.

## Operating Modes

- `demo`: fully simulated, no router required
- `live`: uses the router, SSH scripts, portal server, and incoming device/traffic events

Launch modes:

- no arguments -> `--configure`
- `--configure` -> Setup
- `--show` -> runtime directly
- `--design` -> runtime directly

## Screen Map

1. `Principal`
   - dashboard and router control surface
   - SSH consoles and operative status

2. `Dispositivos + trafico`
   - connected devices
   - raw traffic
   - portal URL banner
   - captured credential reveal

3. `Mapa / conexiones`
   - world map
   - packet trails
   - selected-device events

4. `Perfil de riesgo`
   - score
   - risk band
   - categories
   - services
   - factors and operator summary

5. `Analisis de cifrado`
   - controlled terminal playback
   - typed intro text
   - animated brute-force phase
   - failure result panel

## Current Constraints And Expectations

- Keep the current functionality unless a change is needed for navigation, startup mode, setup, or visual structure.
- Do not reintroduce automatic Demo screen switching.
- Do not make the demo watermark selectable or focusable.
- Keep Screen 5 as the only screen that resets when entered.
- Keep the beep restriction: sounds should only occur when the relevant screen is active.
- Keep the operational log free of credential values and raw traffic payloads.
- Keep stdout protocol lines for orchestration.

## Live Mode Requirements

Live mode depends on a private GL.iNet GL-MT300N-V2 router and the companion scripts.

Required endpoints:

- SSH: `root@192.168.8.1`
- traffic events: port `5555`
- device events: port `5556`
- portal: port `8080`

The app starts and stops the router-side scripts through SSH. The host must be reachable from the router for the traffic and device feeds to work.

## Packaging

Release zips are made with:

```powershell
.\package-release.ps1
.\package-release.ps1 -Force
```

That script:

1. checks the current commit against `releases.json`
2. builds the Release target
3. stages the executable, Qt runtime, plugins, and runtime resources
4. creates `dist\cybershow-wifi-vNN.zip`
5. records the release in `releases.json`
6. creates a matching git tag

The packaged zip also includes `RUNBOOK.md` for show-day live-mode operation.

## Important Source Files

- `src/main.cpp` - CLI parsing, validation, startup mode handling
- `src/InitScreen.cpp` - setup screen
- `src/MainWindow.cpp` - all five runtime screens, event flow, demo logic
- `src/MapView.cpp` - map rendering and packet animation
- `src/WifiPortalServer.cpp` - fake portal server
- `src/TcpJsonLineServer.cpp` - newline-delimited JSON server
- `src/cybershow/common/` - orchestrator protocol and operational log helpers
- `src/cybershow/ui/` - shared Cybershow panels, background, navigation

## Visual Standard Summary

The app should keep the Cybershow look:

- dark technical background
- Spanish operator-facing UI
- common bottom navigation
- operative dashboards rather than decorative cards
- monospaced terminals
- clear panel hierarchy
- screen titles and labels that fit the available space
- scenic treatment only where the screen is meant to be theatrical

Public Wi-Fi is the reference operative app in this family. The layout should stay quiet, dense, and projector-safe.

## What To Check Before Any Future Change

- Does it preserve current runtime behavior?
- Does it keep setup, show, and design modes consistent with the Cybershow standard?
- Does it break operator navigation?
- Does it change live mode dependencies?
- Does it alter the packaging workflow?
- Does it add or remove a screen-level reset, beep, or demo-only behavior?

## Deferred / Known Follow-Ups

- No extra standards markdown files should remain after consolidation.
- Keep the repo documentation surface small: `README.md` for user/operator documentation, `CLAUDE.md` for working context.
- Future work should stay aligned with the current Cybershow conventions rather than reintroducing older app-specific standards docs.
