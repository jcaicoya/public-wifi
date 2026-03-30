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
| 1/M | Main | Cuarzito mascot + 4 SSH consoles + router controls |
| 2/D | Devices | Device list + raw traffic log + credential capture banner |
| 3/N | Navigation | SVG world map (QSvgRenderer) + animated cyan packet trails |
| 4/S | Statistics | Per-device service counts and timestamps |
| 5/E | Encryption | Theatrical WhatsApp brute-force → always fails (E2EE) |

Left/Right arrows cycle screens.

## Key Source Files

| File | Role |
|------|------|
| `src/ShowConfig.h` | Pure data struct: Mode (Normal/Demo) + actSequence bool |
| `src/InitScreen.h/.cpp` | Startup QDialog — TECHNICAL SETUP (fullscreen, two-column) |
| `src/main.cpp` | Entry: `validateResources()`, InitScreen, MainWindow |
| `src/MainWindow.h/.cpp` | All 5 screens, event processing, demo logic, act sequence |
| `src/MapView.h/.cpp` | SVG map, packet animations, region highlights |
| `src/WifiPortalServer.h/.cpp` | Minimal HTTP server (port 8080) — fake WiFi portal |
| `src/TcpJsonLineServer.h/.cpp` | TCP server, newline-delimited JSON |
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
- Act Sequence pauses when entering Screen E; resumes 5 s after animation completes.
- `m_routerRetryTimer` stopped in Demo to prevent blocking `waitForConnected()` freezes.
- Background devices connect/disconnect throughout the loop; DemoPhone always stays connected.
- `demo_events.json` includes a `{"type":"credential",…}` event that triggers the Screen B reveal.

## Visual Design Tokens
- Background: `#090C10` (map/main), `#202020` (lists/consoles)
- Device: `#00FF44` cyber green  |  Packets: `#00FFFF` cyan  |  Alert: `#FF3333` red
- Fonts: Consolas/monospace for terminals; bold for map labels
- Target audience is non-technical — projector legibility over code elegance

## SVG Projection (virtual 1000×600 ↔ lon/lat)
viewBox `82.992 45.607 2528.57 1238.92` — pseudo-equirectangular.
Origin: Oviedo, Asturias, Spain (`svgCoord(-5.845, 43.361)`).

## Tools (standalone Qt apps)
- `tools/polygon-editor/` → target `polygon_editor` — edit region polygons → saves `resources/regions.json`
- `tools/service-mapper/` → target `service_mapper` — edit service→region mapping → saves `resources/services.json`
