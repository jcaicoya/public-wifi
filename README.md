# Public Wi-Fi Cybershow

Live stage demonstration of public Wi-Fi privacy risks. A controlled phone connects to a GL.iNet router; the app visualizes network metadata in real time for a non-technical audience. No real attacks — educational and ethical.

---

## Current State

- **Init screen** — operator selects Normal (live router) or Demo mode, with optional Act Sequence (unattended full-show cycle)
- **Screen A** — 4 live SSH consoles (device watch, traffic, syslog, connections)
- **Screen B** — device list + raw traffic log + **credential capture banner** (shows name/email when volunteer submits the WiFi portal form)
- **Screen C** — SVG world map with animated packet trails from Asturias, Spain
- **Screen D** — per-device service stats (counts, active time)
- **Screen E** — theatrical WhatsApp brute-force attempt → always fails → E2EE reassurance
- **WiFi Portal** — fake login page served on port 8080; captured credentials shown dramatically on Screen B
- **Demo mode** — fully simulated show (no router needed), 5 background devices, Act Sequence auto-cycling

---

## Next Steps

1. **Color-code traffic by service on Screen B** — BANKING=red, WHATSAPP=cyan, etc. Zero logic change, big visual impact.
2. **Service label pulse at destination on Screen C** — currently the label flies with the packet; pulsing at the region when it lands reads better on a projector.
3. **Danger score on Screen D** — a 0–100 risk bar computed from services seen (BANKING+VPN = high, SEARCH = low).
4. **Smoother act sequence transitions** — brief fade between screens for unattended demo.
5. **Sound effect on device connect** — subtle ping via QSoundEffect.

---

## Packaging a Release

Use `package-release.ps1` to build a distributable zip and record it in `releases.json`.

```powershell
# Standard: build + zip if the commit has changed
.\package-release.ps1

# Force repackage even if the commit is the same
.\package-release.ps1 -Force
```

The script:

1. Compares the current `HEAD` commit against the last entry in `releases.json`
2. If different, builds the Release target and creates `dist\cybershow-wifi-vNN.zip`
3. Appends an entry to `releases.json` (commit hash, date, message, zip name)
4. Creates a local git tag (`v00`, `v01`, …)

After packaging, push the new tag to the remote:

```powershell
git push --tags
```

The zip in `dist\` is self-contained for Windows and includes the runtime resource files
(`resources/regions.json`, `resources/services.json`) required at startup.
The target machine must have the
[Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) installed.

`dist\` is gitignored — only `releases.json` and the git tags are committed.

## Build

Requirements: Qt 6.7.3, CMake 3.28+, MSVC (Windows).

```bash
# CLion: open project, build target public_wifi
# Or manually:
cmake -S . -B build
cmake --build build
```

The build copies Qt DLLs and the platform plugin next to the executable automatically.

---

## Run

```
public_wifi.exe
```

At the **TECHNICAL SETUP** screen choose:
- **Normal** — connects to router at `192.168.8.1`
- **Demo** — simulated show, no router needed
  - **Act Sequence** — unattended full-show cycle (7 s/screen)

---

## Show Flow (operator guide)

| Step | Screen | Action |
|------|--------|--------|
| 1 | A (Main) | Launch app, select mode, press **START SHOW** |
| 2 | B (Devices) | Ask volunteer to connect to the show Wi-Fi. Show the portal URL on screen (`http://192.168.8.X:8080`). Volunteer fills in name + email → credential banner appears |
| 3 | C (Map) | Click the volunteer's device in the list → map shows live packet trails |
| 4 | D (Stats) | Press `4` — show the behavioral profile built in real time |
| 5 | E (Encryption) | Press `5` — brute-force attempt runs automatically, ends with "13.8 Billion Years" |

Keyboard shortcuts: `1–5` or `M/D/N/S/E` to jump to any screen; `←/→` to cycle.

## Operations

The app emits machine-readable `CYBERSHOW_*` lines on stdout for supervision. Human diagnostics and operational events are written separately.

Operational log:

```text
logs/public-wifi.log
```

Format:

```text
timestamp | public-wifi | launchMode | profile | level | component | message
```

The log records runtime events such as startup, mode entry, screen changes, and server startup errors. Credential values and raw traffic payloads are not written to this log.

Configuration follow-up:

`--config <path>` is reserved and stored internally, but JSON config loading is deferred. Current runtime configuration still comes from setup controls and CLI profile/window flags.

---

## Router Scripts (GL.iNet GL-MT300N-V2, OpenWrt)

SSH access: `ssh root@192.168.8.1` (passwordless key required).

```bash
# Start traffic events (on router)
/root/send_traffic_events.sh <PC_IP> 5555 &

# Start device watch (on router)
/root/device_watch.sh <PC_IP> 5556 &
```

The app starts/stops these automatically via SSH when you use the buttons on Screen A (Normal mode).

---

## Ethical Constraints

- Never inspects audience devices
- Uses a dedicated private router
- Only monitors the controlled show phone
- Purpose: education and awareness
