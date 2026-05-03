# APP_SPEC - Public Wi-Fi Cybershow

Version: 0.1
Status: implemented refactor contract for Cybershow standard v0.3; pending operator compile/manual validation.

This file contains the app-specific contract for adapting Public Wi-Fi to the Cybershow Qt application standard. It must be read together with:

- `cybershow_app_standards_v0_3/QT_APP_LOOK_AND_FEEL.md`
- `cybershow_app_standards_v0_3/CYBERSHOW_APP_CONVENTIONS.md`
- `cybershow_app_standards_v0_3/CYBERSHOW_REFACTOR_CHECKLIST.md`
- `cybershow_app_standards_v0_3/PUBLIC_WIFI_VISUAL_FIXES.md`

The refactor goal is to unify startup, navigation, setup, visual structure, and operator language while preserving the current show behavior.

## 1. Application Identity

- Visible name: `Public Wi-Fi Cybershow`
- Internal name: `public-wifi`
- Executable: `public_wifi.exe`
- Role in the show: live theatrical visualization of public Wi-Fi privacy risks using a controlled phone and private GL.iNet router.
- Main type: mixed, mostly operative dashboards with one scenic encryption screen.
- Target audience: operator plus non-technical audience watching projected screens.

## 2. Screen Contract

| No. | ID | Long title | Short title | Type | Notes |
|---:|---|---|---|---|---|
| 1 | `principal` | `Centro de control Cybershow` | `Principal` | operative | Router controls, SSH consoles, status panel. |
| 2 | `dispositivos` | `Dispositivos + trafico` | `Dispositivos` | operative | Device list, raw traffic, portal URL, credential reveal. |
| 3 | `mapa` | `Mapa / conexiones` | `Mapa` | operative | SVG world map, packet trails, selected-device events. |
| 4 | `riesgo` | `Perfil de riesgo` | `Riesgo` | operative | Per-device service stats and risk explanation. |
| 5 | `cifrado` | `Analisis de cifrado` | `Cifrado` | scenic | Theatrical WhatsApp brute-force attempt that always fails. |

Current code names:

- `PageId::Main` maps to screen 1.
- `PageId::Devices` maps to screen 2.
- `PageId::Navigation` maps to screen 3.
- `PageId::Statistics` maps to screen 4.
- `PageId::Encryption` maps to screen 5.

## 3. Setup

Setup is a technical preparation screen. It should not be staged as a public screen.

Target setup parameters:

- Operating profile: `live`, `demo`, and optionally `dev`.
- Live mode target router: GL.iNet GL-MT300N-V2 at `192.168.8.1`.
- Demo mode source: embedded `demo_events.json`.
- Demo mode includes a non-interactive pulsing `DEMO` indicator at center-right during runtime.
- Window mode where supported: fullscreen or windowed.

Current setup parameters:

- Run mode combo:
  - `Normal mode`: connects to the router at `192.168.8.1`.
  - `Demo mode`: uses simulated events from `demo_events.json`.
- Demo mode navigation is always controlled by the operator.

Target setup rules:

- No arguments must behave like `--configure`.
- `--configure` starts in setup and allows returning to setup with `Esc`.
- `--design` and `--show` start directly in execution.
- Setup must be inaccessible in `--design` and `--show`.
- `Enter`, `Space`, `Right Arrow`, `1`, and the primary button start execution from setup when focus is not inside an editable control.
- The primary setup button must be blue and named `INICIAR SHOW`.
- Setup must use a centered card over the common Cybershow background.
- Setup must not show a fixed side mascot or public QR.

## 4. Launch Mode And Operating Mode

Launch mode and operating mode are separate concepts.

Launch modes:

- `--configure`: show setup first.
- `--design`: skip setup and run execution mode directly.
- `--show`: skip setup and run execution mode directly.

Operating modes:

- `live`: controlled real router and show phone.
- `demo`: fully simulated show, no router required.
- `dev`: development profile, if later needed.

Mapping target:

- Setup `Normal mode` maps to operating profile `live`.
- Setup `Demo mode` maps to operating profile `demo`.
- CLI `--profile live` maps to `ShowConfig::Mode::Normal`.
- CLI `--profile demo` maps to `ShowConfig::Mode::Demo`.
- Demo operation does not provide unattended automatic screen cycling.

## 5. Demo And Live Behavior

Demo data:

- Embedded `resources/demo_events.json`.
- Simulated device connect/disconnect events.
- Simulated traffic events.
- Simulated credential event for the Wi-Fi portal reveal.
- DemoPhone remains available as the stable controlled target.
- Screen navigation remains operator-controlled during demo playback.

Live controlled data:

- Router traffic events on port `5555`.
- Router device events on port `5556`.
- Local Wi-Fi portal on port `8080`.
- SSH scripts started on the GL.iNet router from Screen 1 controls.

Functional constraints:

- Do not inspect audience devices.
- Do not add real attack behavior.
- Keep the WhatsApp brute-force attempt theatrical and guaranteed to fail.
- Preserve existing TCP, portal, router, demo, map, and stats event pipelines unless a narrow adaptation is required for standard navigation or display.

## 6. Navigation Contract

Runtime navigation:

- `Right Arrow`: next screen, non-circular.
- `Left Arrow`: previous screen, non-circular.
- `1` to `5`: direct screen selection.
- Bottom navigation click: same as pressing the matching number.
- `Esc`: return to setup only when launched with `--configure`.
- `Esc`: no effect in `--show` or `--design`.
- `Alt+F4`: standard system close behavior.

Navigation removals:

- Remove primary navigation shortcuts by letter: `M`, `D`, `N`, `S`, `E`.
- Do not use `Esc` as a quit shortcut.
- Do not add `Q`, `Ctrl+Q`, or similar show-operation quit shortcuts.

Bottom navigation labels:

- `1 - Principal`
- `2 - Dispositivos`
- `3 - Mapa`
- `4 - Riesgo`
- `5 - Cifrado`

## 7. Visual Contract

Common visual rules:

- Use the Cybershow common background.
- Use the common dark technical palette.
- Use common panel, metric, alert, and bottom navigation components where practical.
- Use Spanish for operator and audience-facing UI text.
- Keep logs and technical payloads in English when they represent router/script output or technical traces.
- Prioritize projector legibility over decorative density.

Screen-specific visual targets:

- Screen 1 must replace the right-side mascot/identity block with a real status panel.
- Screen 2 must present the portal URL as a clean info banner and the credential reveal as a normalized critical alert.
- Screen 3 must keep the map and packet trails, with translated labels and improved destination readability.
- Screen 4 must add explanatory risk content below the main score.
- Screen 5 must keep the terminal/scenic style but improve Spanish copy and readability.

## 8. Orchestrator Integration

Minimum stdout messages:

```text
CYBERSHOW_STATUS READY
CYBERSHOW_STATUS RUNNING
CYBERSHOW_SCREEN <n> <id>
CYBERSHOW_STATUS ERROR <code>
CYBERSHOW_STATUS FINISHED
```

Implementation note:

- `READY`, `RUNNING`, `SCREEN`, and relevant `ERROR` messages are implemented.
- `FINISHED` is reserved for a future explicit show lifecycle event; normal OS/app exit is still handled by Qt.

Required screen messages:

```text
CYBERSHOW_SCREEN 1 principal
CYBERSHOW_SCREEN 2 dispositivos
CYBERSHOW_SCREEN 3 mapa
CYBERSHOW_SCREEN 4 riesgo
CYBERSHOW_SCREEN 5 cifrado
```

Possible app-specific events:

```text
CYBERSHOW_EVENT device_connected <controlled-or-demo-device-id>
CYBERSHOW_EVENT credential_demo <sanitized-demo-identity>
CYBERSHOW_EVENT encryption_demo_finished failed
```

Rules:

- Stdout should be reserved for `CYBERSHOW_*` protocol lines where possible.
- Human diagnostics should go to stderr, UI logs, or file logs.
- The orchestrator must not depend on visual UI strings.

## 9. Logging And Configuration

Target log format:

```text
timestamp | public-wifi | launchMode | profile | level | component | message
```

Target log file:

```text
logs/public-wifi.log
```

Current implementation:

- The app writes operational events to `logs/public-wifi.log` under the executable directory.
- Log lines use the target format:
  `timestamp | public-wifi | launchMode | profile | level | component | message`.
- Credential capture is logged only as an occurrence; captured name/email values are intentionally omitted.
- Raw traffic payloads are not written to the operational log.

Configuration target:

```text
config/public-wifi.json
```

Initial configuration scope:

- `profile`
- `window.fullscreen`
- `window.screen`
- `network.routerHost`
- `network.trafficPort`
- `network.devicePort`
- `network.portalPort`

Deferred configuration work is allowed if documented in this spec.

Deferred configuration status:

- `--config <path>` is parsed and stored in `ShowConfig::configPath`, but JSON loading is not implemented yet.
- Until JSON loading is added, setup controls and CLI profile/window options remain the source of runtime configuration.

## 10. Resource Contract

Required embedded resources:

- `:/world_map.svg`
- `:/flying-cuarzito.png`
- `:/demo_events.json`

Required filesystem resources:

- `resources/regions.json`
- `resources/services.json`

Startup must fail clearly if required resources are missing or corrupted.

## 11. Security And Ethics

- Only use a dedicated private router.
- Only monitor the controlled show phone.
- Do not inspect audience devices.
- Do not show accidental personal data.
- Demo data must be simulated or consented.
- Credential capture is theatrical and controlled through the local Wi-Fi portal.
- Sensitive display data must be sanitized before appearing in public screens.
- External or destructive actions must remain controlled from setup/operator surfaces.

## 12. Allowed Exceptions

- The embedded Cuarzito image may remain as an application resource if used in packaging or future identity screens, but it should not be a fixed side element in setup or operative runtime screens.
- Screen 5 may be more scenic than the other screens because it is the public E2EE reassurance moment.
- Technical logs may remain in English when they represent terminal/router output.
- Live router operation may keep app-specific controls if they are needed for the GL.iNet scripts.

## 13. Refactor Checklist

- [x] CLI common launch modes implemented.
- [x] No arguments behave like `--configure`.
- [x] `--configure` starts in setup.
- [x] `--design` and `--show` start in execution.
- [x] Setup is inaccessible in `--design` and `--show`.
- [x] Navigation common behavior implemented.
- [x] Letter shortcuts removed for primary navigation.
- [x] Common bottom navigation implemented.
- [x] Common background applied.
- [x] Setup redesigned as a centered technical card.
- [x] States and labels normalized.
- [x] Operator-facing text translated to Spanish.
- [x] Minimum `CYBERSHOW_*` stdout protocol implemented.
- [x] Internal logging reviewed.
- [x] Sensitive data display reviewed.
- [x] Existing live/demo functionality preserved by code review; compile/manual validation remains operator-owned.

## 14. Known Exceptions And Deferred Work

- `CYBERSHOW_STATUS FINISHED` is reserved but not emitted yet because the app does not currently model an explicit show-finished lifecycle separate from normal process exit.
- `--config <path>` is parsed and retained in `ShowConfig::configPath`, but JSON config loading is deferred.
- Router host and port values remain hardcoded to the existing show setup (`192.168.8.1`, `5555`, `5556`, `8080`) until persistent configuration is implemented.
- Human-readable Qt diagnostics may still go through Qt's debug/warning output; stdout protocol lines are normalized with `CYBERSHOW_*`.
- Final compile, launch-mode validation, projection check, and live router smoke tests are operator-owned.
