# CLAUDE.md — Public Wi-Fi Cybershow

Educational theatrical demo of public Wi-Fi privacy risks. Controlled phone on a private GL.iNet router; Qt app visualizes metadata live for an audience. No real attacks.

## Stack
- **C++23 / Qt 6.7.3** (Widgets, Network, Svg) / CMake 3.28+ / MSVC / CLion / Windows 11
- **Qt path:** `C:/Qt/6.7.3/msvc2022_64`  |  **Build dirs:** `cmake-build-debug/`, `build/`

## Architecture
```
Phone → GL.iNet GL-MT300N-V2 (OpenWrt) → router scripts
  → port 5555  traffic events  (SEARCH, BANKING, WHATSAPP …)
  → port 5556  device events   (connected / disconnected)
  → port 8080  HTTP portal     (fake WiFi login — credential capture)
  → TcpJsonLineServer / WifiPortalServer → MainWindow
```
**Demo mode:** replays `demo_events.json` via QTimer (600 ms/event) — identical pipeline, no router needed.

## The 5 Screens

| Key | PageId | Screen |
|-----|--------|--------|
| 1 | Main | Centro de control Cybershow: 4 SSH consoles, router controls, operative status |
| 2 | Devices | Dispositivos + trafico: device list, raw router messages, portal URL, credential banner |
| 3 | Navigation | Mapa / conexiones: SVG world map, packet trails, selected-device events |
| 4 | Statistics | Perfil de riesgo: score, categories, risk factors, timeline, operator explanation |
| 5 | Encryption | Analisis de cifrado: controlled brute-force demo, always fails, E2EE reassurance |

Runtime navigation: `1-5`, bottom bar clicks, and Left/Right without wrapping. Letter shortcuts were removed. `Esc` returns to setup only in `--configure`.

## Key Source Files

| File | Role |
|------|------|
| `src/ShowConfig.h` | Pure data struct: Mode (Normal/Demo) + actSequence bool |
| `src/InitScreen.h/.cpp` | Spanish Cybershow technical setup card |
| `src/main.cpp` | Entry: CLI mode parsing, `validateResources()`, setup/runtime loop, orchestrator status |
| `src/MainWindow.h/.cpp` | All 5 screens, event processing, demo logic, act sequence |
| `src/MapView.h/.cpp` | SVG map, packet animations, region highlights |
| `src/WifiPortalServer.h/.cpp` | Minimal HTTP server (port 8080) — fake WiFi portal |
| `src/TcpJsonLineServer.h/.cpp` | TCP server, newline-delimited JSON |
| `src/cybershow/common/` | Shared launch, navigation, orchestrator, and operational log helpers |
| `src/cybershow/ui/` | Shared Cybershow theme, background, panels, bottom navigation |
| `resources/demo_events.json` | Demo event sequence (traffic + device + credential events) |
| `resources/regions.json` | Region polygon data (editable via polygon-editor tool) |
| `resources/services.json` | Service→Region mapping (editable via service-mapper tool) |

## Resource Files (all required at startup)
`validateResources()` in `main.cpp` checks all five — fatal error dialog if any missing. No hardcoded fallback data.

| File | Embedded? |
|------|-----------|
| `:/world_map.svg` | Qt resource |
| `:/flying-cuarzito.png` | Qt resource |
| `:/demo_events.json` | Qt resource |
| `resources/regions.json` | Filesystem |
| `resources/services.json` | Filesystem |

## Demo Mode Details
- **Init screen** selects Normal or Demo; optionally enables **Act Sequence** (unattended 7 s/screen cycle).
- Act Sequence pauses when entering Screen 5; resumes 5 s after animation completes.
- `m_routerRetryTimer` stopped in Demo to prevent blocking `waitForConnected()` freezes.
- Background devices connect/disconnect throughout the loop; DemoPhone always stays connected.
- `demo_events.json` includes a `{"type":"credential",…}` event that triggers the Screen 2 reveal.

## Runtime Modes And Protocol
- No arguments and `--configure` open setup.
- `--show` and `--design` skip setup and enter runtime directly.
- `--profile live` maps to Normal/live router mode; `--profile demo` maps to Demo.
- Stdout emits `CYBERSHOW_STATUS READY`, `CYBERSHOW_STATUS RUNNING`, `CYBERSHOW_SCREEN <n> <id>`, and startup/server `CYBERSHOW_STATUS ERROR <code>` lines.
- Operational log path: `logs/public-wifi.log`.
- Log format: `timestamp | public-wifi | launchMode | profile | level | component | message`.
- Credential values and raw traffic payloads are not written to the operational log.

## Visual Design Tokens
- Common Cybershow dark technical background and bottom navigation.
- Shared panel object names: `CyberPanel`, `CyberPanelRaised`, `CyberPanelCritical`.
- Status object names: `StatusOk`, `StatusInfo`, `StatusWarning`, `StatusError`.
- Fonts: Consolas/monospace for terminals; Segoe UI/Inter/Arial for common UI.
- Target audience is non-technical; projector legibility over decorative density.

## SVG Projection (virtual 1000×600 ↔ lon/lat)
viewBox `82.992 45.607 2528.57 1238.92` — pseudo-equirectangular.
Origin: Oviedo, Asturias, Spain (`svgCoord(-5.845, 43.361)`).

## Packaging

Script: `package-release.ps1`
Tracking file: `releases.json` (committed to git)
Output folder: `dist\` (gitignored)
Zip naming: `cybershow-wifi-vNN.zip` (zero-padded, incrementing)

Workflow:
- `.\package-release.ps1` — builds Release, zips, appends to releases.json, creates git tag
- `.\package-release.ps1 -Force` — same but skips commit-change check
- `git push --tags` — push tags to remote after packaging

Zip contents: `public_wifi.exe` + 6 Qt DLLs (Core, Gui, Multimedia, Network, Widgets, Svg)
+ `plugins/platforms/qwindows.dll` + `plugins/multimedia/windowsmediaplugin.dll`
+ `resources/regions.json` + `resources/services.json` (runtime files, not embedded).

Target machine requires Visual C++ Redistributable (install once).

## Tools (standalone Qt apps)
- `tools/polygon-editor/` → target `polygon_editor` — edit region polygons → saves `resources/regions.json`
- `tools/service-mapper/` → target `service_mapper` — edit service→region mapping → saves `resources/services.json`
