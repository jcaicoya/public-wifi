# CLAUDE.md — Public Wi-Fi Cybershow

Educational theatrical demo of public Wi-Fi privacy risks. A controlled phone connects to a GL.iNet OpenWrt router; this Qt app visualizes network metadata in real time for a live audience. No real attacks — only a controlled device on a private network.

---

## Tech Stack

- **Language:** C++23
- **Framework:** Qt 6.7.3 (Widgets, Network, Core, Gui)
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
  MapView.h/.cpp        — custom QPainter world map with animated packet trails
  TcpJsonLineServer.h/.cpp — TCP server that reads newline-delimited JSON
resources/
  resources.qrc         — Qt resource file
  demo_events.json      — replay events for Demo Mode
  flying-cuarzito.png   — mascot image
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
    port 5555 → traffic events  (type: SEARCH, AMAZON, WHATSAPP, MICROSOFT, APPLE, VIDEO)
    port 5556 → device events   (type: connected, disconnected)
  → TcpJsonLineServer → MainWindow::processTrafficEvent / processDeviceEvent
```

**Demo Mode:** `--demo` CLI flag bypasses the router entirely. Replays `demo_events.json` via `QTimer` every 1.5 s, injecting into the same processing pipeline.

---

## The 5 Screens

| ID | Key | `PageId` enum | Description |
|----|-----|---------------|-------------|
| A | `1` / `M` | `Main` | Startup screen: Cuarzito mascot + 4 SSH hacker consoles, router connect/start/stop buttons |
| B | `2` / `D` | `Devices` | Device list (left) + raw scrolling traffic log (right) |
| C | `3` / `N` | `Navigation` | Low-poly world map (`MapView`) with animated cyan packet trails + filtered event log |
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

### `MapView`
- Custom `QWidget` drawing a low-poly geometric world map via `QPainterPath`
- Origin fixed at Asturias, Spain; packets animate to named geographic regions
- Particle trails fade over time; regions briefly highlight on packet arrival
- Service → region mapping: SEARCH/APPLE/MICROSOFT → North America, WHATSAPP → North Europe, AMAZON → Europe, VIDEO → Asia

### `TcpJsonLineServer`
- Listens on a TCP port, emits `lineReceived(QByteArray)` for each newline-delimited JSON line

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
2. **World map:** More geographic regions, better SVG coordinates, pulsing datacenter nodes
3. **Event classification:** More services (TikTok, Instagram, etc.)
4. **UI polish:** Smoother screen transitions, projector readability
5. **TCP stability:** Reconnection logic if router drops mid-show; optimize `MapView` repaints
6. **Unify message model:** Consistency across components
7. *(Optional)* `QSoundEffect` for device connect/disconnect events

---

## Router

- **Device:** GL.iNet GL-MT300N-V2 (Mango) running OpenWrt
- **SSH:** `ssh root@192.168.8.1` (passwordless key required for auto-control)
- The app starts/stops router scripts via SSH `QProcess` in the background