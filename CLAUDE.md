# CLAUDE.md — Public Wi-Fi Cybershow

Educational theatrical demo of public Wi-Fi privacy risks. A controlled phone connects to a GL.iNet OpenWrt router; this Qt app visualizes network metadata in real time for a live audience. No real attacks — only a controlled device on a private network.

---

## Tech Stack

- **Language:** C++23
- **Framework:** Qt 6.7.3 (Widgets, Network, Core, Gui, **Svg**)
- **Build:** CMake 3.28+, MSVC, CLion on Windows 11
- **Qt path:** `C:/Qt/6.7.3/msvc2022_64`
- **Build dirs:** `cmake-build-debug/` (CLion/Ninja), `build/` (MSVC/Visual Studio)

---

## Project Structure

```
src/
  main.cpp              — entry point, parses --demo flag
  MainWindow.h/.cpp     — central controller, all 5 screens, event processing
  ScreenPage.h/.cpp     — thin QWidget wrapper per screen
  MapView.h/.cpp        — SVG world map with animated packet trails and region highlights
  TcpJsonLineServer.h/.cpp — TCP server that reads newline-delimited JSON
resources/
  resources.qrc         — Qt resource file
  demo_events.json      — 40 replay events for Demo Mode (15 service types)
  flying-cuarzito.png   — mascot image
  world_map.svg         — BlankMap-World-Compact SVG (borders-only, cyan-blue)
  regions.json          — runtime polygon data written by polygon-editor tool (optional)
tools/
  polygon-editor/       — standalone Qt app for interactively editing map region polygons
    main.cpp / PolygonEditor.h/.cpp / MapCanvas.h/.cpp
    editor_resources.qrc / CMakeLists.txt
scripts/                — OpenWrt shell scripts (run on the router)
  device_watch.sh       — emits device connect/disconnect events on port 5556
  send_traffic_events.sh — emits traffic events on port 5555
  sniff_payload.sh      — (exploratory) HTTP payload capture
```

---

## Architecture

```
Phone → GL.iNet GL-MT300N-V2 (OpenWrt) → router scripts
  → JSON events over TCP
    port 5555 → traffic events  (type: SEARCH, AMAZON, WHATSAPP, MICROSOFT, APPLE, VIDEO, ...)
    port 5556 → device events   (type: connected, disconnected)
  → TcpJsonLineServer → MainWindow::processTrafficEvent / processDeviceEvent
```

**Demo Mode:** `--demo` CLI flag bypasses the router entirely. Replays `demo_events.json` via `QTimer` every **0.6 s**, injecting into the same processing pipeline. A toggle button on Screen C also starts/stops demo mode at runtime.

---

## The 5 Screens

| ID | Key | `PageId` enum | Description |
|----|-----|---------------|-------------|
| A | `1` / `M` | `Main` | Startup screen: Cuarzito mascot + 4 SSH hacker consoles, router connect/start/stop buttons |
| B | `2` / `D` | `Devices` | Device list (left) + raw scrolling traffic log (right) |
| C | `3` / `N` | `Navigation` | SVG world map (`MapView`) with animated cyan packet trails + filtered event log + Demo Mode button |
| D | `4` / `S` | `Statistics` | Aggregated per-device stats (service counts, active time) |
| E | `5` / `E` | `Encryption` | Theatrical WhatsApp brute-force attempt that always fails → E2EE reassurance |

Navigation also works with Left/Right arrow keys (cycle screens).
Clicking a device on Screen B auto-navigates to Screen C.

---

## Key Classes

### `MainWindow`
- Owns all screens, both `TcpJsonLineServer`s, SSH `QProcess`es, demo timer
- `processTrafficEvent()` / `processDeviceEvent()` — central event dispatch
- `DeviceStats` struct tracks per-MAC service counts and timestamps
- `startEncryptionDemo()` / `updateEncryptionAnimation()` — Screen E sequence
- **Demo mode fix:** `m_routerRetryTimer` is stopped when demo starts — prevents periodic 2s freezes caused by `waitForConnected()` blocking the main thread

### `MapView`
- Renders `world_map.svg` (borders-only, cyan-blue) via `QSvgRenderer` as pre-rendered `QPixmap` background
- `svgCoord(lon, lat)` — static projection function aligning virtual 1000×600 space with the SVG's pseudo-equirectangular layout
- `buildMapRegions()` — defines 11 named `QPainterPath` polygons (hardcoded fallback)
- `tryLoadRegionsFromFile()` — on startup, overrides hardcoded polygons with `resources/regions.json` if found
- Origin fixed at Oviedo, Asturias, Spain (`svgCoord(-5.845, 43.361)`)
- Regions: North America, South America, Western Europe, Northern Europe, Russia/CIS, Africa, Middle East, South Asia, East Asia, Oceania, **Mediterranean**
- Service → region mapping (18 entries): SEARCH/APPLE/SOCIAL/MAPS → North America; WHATSAPP/MICROSOFT → Northern Europe; AMAZON/EMAIL/BANKING/NEWS/CDN/SHOPPING → Western Europe; VIDEO/STREAMING/GAMING → East Asia; VPN → Russia/CIS; TELEMETRY → South Asia; REGIONAL → Middle East; LATAM → South America; PACIFIC → Oceania
- **Performance:** `resizeEvent` debounced via 150ms single-shot timer — prevents SVG re-render spikes during layout shifts
- Packet speed: `progress += 0.015` per 30ms frame (~33 FPS)

### `TcpJsonLineServer`
- Listens on a TCP port, emits `lineReceived(QByteArray)` for each newline-delimited JSON line

### `MapCanvas` (polygon-editor tool)
- Interactive Qt widget for editing region polygons over the SVG map background
- Same `svgCoord()` / `virtualToLonLat()` projection as `MapView`
- `saveToJson()` / `loadFromJson()` — persists polygon data to `resources/regions.json`
- Drag nodes, left-click edge to insert node, right-click node to delete

---

## SVG Projection

The SVG uses a pseudo-equirectangular projection (viewBox `82.992 45.607 2528.57 1238.92`):

```
x_svg = (lon + 180) × 7.024
y_svg = (90 − lat) × 8.35
x_virtual = (x_svg − 82.992) / 2528.57 × 1000
y_virtual = (y_svg − 45.607) / 1238.92 × 600
```

Inverse (virtual → lon/lat):
```
lon = (x_virtual × 2528.57/1000 + 82.992) / 7.024 − 180
lat = 90 − (y_virtual × 1238.92/600 + 45.607) / 8.35
```

---

## Polygon Editor Tool

Standalone Qt app (`tools/polygon-editor/`), built as separate CMake target `polygon_editor`.

**Workflow:**
1. Run `polygon_editor`
2. Select region from left panel
3. Drag nodes / left-click edge to insert / right-click to delete
4. Click **💾 Save regions.json** — writes to `[project_root]/resources/regions.json`
5. Run `public_wifi` — loads the file automatically on startup

**Save path resolution (both apps):**
1. `QDir::currentPath()/resources/regions.json` — works when CLion working dir = project root
2. Relative to executable: `../resources/`, `../../resources/`
3. Fallback: next to executable

---

## Visual / Theatrical Design

- **Aesthetic:** Dark cyber dashboard — looks like a hacker analyst tool
- **Background:** `#090C10` (map/main), `#202020` (lists/consoles)
- **Device color:** `#00FF44` cyber green
- **Packet/data color:** `#00FFFF` cyan
- **Alert/failure:** `#FF3333` red
- **Fonts:** Consolas / monospace for terminals and IPs; bold for map labels
- **Target audience:** Non-technical live audience — legibility on projectors matters

---

## Screen E — Encryption Demo Flow

1. **Interception phase:** Terminal shows intercepted packet to `whatsapp.net`, floods screen with garbled hex payload
2. **Brute-force animation (~10 s):** Fast-scrolling lines (`Injecting keys...`, `Brute-forcing AES-256...`, `Attempt 1,405,992... Failed.`)
3. **Climax:** Giant overlay — `CRITICAL ERROR: DECRYPTION FAILED. Estimated time to crack: 13.8 Billion Years. STATUS: END-TO-END ENCRYPTED.`

---

## Open Milestones

1. **Screen E contrast demo:** Capture plain HTTP credentials on Screen B first, then attempt WhatsApp — makes the E2EE failure more dramatic
2. **Mediterranean service mapping:** Assign a service to the new Mediterranean region in `m_serviceToRegion`
3. **Event classification:** More services (TikTok, Instagram, etc.)
4. **UI polish:** Smoother screen transitions, projector readability
5. **TCP stability:** Reconnection logic if router drops mid-show
6. **Unify message model:** Consistency across components
7. *(Optional)* `QSoundEffect` for device connect/disconnect events

---

## Router

- **Device:** GL.iNet GL-MT300N-V2 (Mango) running OpenWrt
- **SSH:** `ssh root@192.168.8.1` (passwordless key required for auto-control)
- The app starts/stops router scripts via SSH `QProcess` in the background
- **Note:** Router retry timer (`m_routerRetryTimer`, 5s interval) is stopped when demo mode starts to prevent main-thread freezes
