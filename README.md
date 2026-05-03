# Public Wi-Fi Cybershow

Live stage demonstration of public Wi-Fi privacy risks. A controlled phone connects to a GL.iNet router; the app visualizes network metadata in real time for a non-technical audience. No real attacks — educational and ethical.

---

## Current State

- **Setup** — Spanish technical setup card; operator selects live/demo mode and optional Act Sequence
- **Screen 1: Principal** — control dashboard with SSH consoles, router controls, and operative status
- **Screen 2: Dispositivos** — connected/known devices, raw router messages, portal URL banner, and credential reveal
- **Screen 3: Mapa** — SVG world map with animated packet trails from Asturias, Spain and destination pulse
- **Screen 4: Riesgo** — per-device risk score, categories, factors, timeline, and operator explanation
- **Screen 5: Cifrado** — controlled WhatsApp brute-force demonstration, always fails, then E2EE reassurance
- **WiFi Portal** — fake login page served on port 8080; captured credentials shown dramatically on Screen 2
- **Demo mode** — fully simulated show (no router needed), 5 background devices, Act Sequence auto-cycling

---

## Refactor Status

Aligned with the Cybershow Qt standard:

- No arguments behave like `--configure`; `--configure` opens setup.
- `--show` and `--design` skip setup and enter runtime directly.
- Setup can start the show with Enter, Space, Right Arrow, `1`, or `INICIAR SHOW`.
- Runtime navigation uses `1-5`, Left/Right, and the common bottom navigation bar.
- Letter shortcuts for primary navigation have been removed.
- Runtime emits `CYBERSHOW_*` stdout protocol lines for orchestration.
- Operational events are logged to `logs/public-wifi.log`.

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

At the **Public Wi-Fi - Configuracion tecnica** screen choose:
- **Normal** — connects to router at `192.168.8.1`
- **Demo** — simulated show, no router needed
  - **Act Sequence** — unattended full-show cycle (7 s/screen)

CLI launch modes:

```text
public_wifi.exe
public_wifi.exe --configure
public_wifi.exe --show --profile demo
public_wifi.exe --design --profile demo --windowed
public_wifi.exe --show --profile live --fullscreen
```

---

## Show Flow (operator guide)

| Step | Screen | Action |
|------|--------|--------|
| 1 | Principal | Launch app, select mode, press **INICIAR SHOW** |
| 2 | Dispositivos | Ask volunteer to connect to the show Wi-Fi. Show the portal URL on screen (`http://192.168.8.X:8080`). Volunteer fills in name + email; the credential banner appears |
| 3 | Mapa | Click the volunteer's device in the list; map shows live packet trails |
| 4 | Riesgo | Press `4`; show the behavioral profile built in real time |
| 5 | Cifrado | Press `5`; brute-force demo runs automatically and ends with the E2EE reassurance |

Keyboard shortcuts: `1-5` jumps to screens. Left/Right move without wrapping. `Esc` returns to setup only when launched with `--configure`.

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

The app starts/stops these automatically via SSH when you use the buttons on Screen 1 (Normal mode).

---

## Ethical Constraints

- Never inspects audience devices
- Uses a dedicated private router
- Only monitors the controlled show phone
- Purpose: education and awareness
