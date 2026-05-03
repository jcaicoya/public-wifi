# Public Wi-Fi Cybershow Refactor Plan

Goal: unify this Qt controller with the Cybershow visual and operative standard while preserving current functionality.

Reference documents:

- `cybershow_app_standards_v0_3/CYBERSHOW_APP_CONVENTIONS.md`
- `cybershow_app_standards_v0_3/QT_APP_LOOK_AND_FEEL.md`
- `cybershow_app_standards_v0_3/CYBERSHOW_REFACTOR_CHECKLIST.md`
- `cybershow_app_standards_v0_3/PUBLIC_WIFI_VISUAL_FIXES.md`
- `cybershow_app_standards_v0_3/templates/APP_SPEC_TEMPLATE.md`

Note: `APP_SPEC.md` is not currently present in this repository. The standards package provides `templates/APP_SPEC_TEMPLATE.md`, so the first refactor step creates the app-specific spec from that template.

Working rule: each numbered step is intended to become one commit. Codex handles source/documentation edits. The operator handles compile, manual test, git, release packaging, deploy, and final show validation after each step.

## Resume Notes

Last updated during Step 11.

Completed commits through Step 4:

- `b7175c7` - `Document Public Wi-Fi app spec`
- `d8fffd5` - `Import Cybershow shared components`
- `a672aec` - `Add show sound effects`
- `d583fea` - `Add standard CLI launch modes`
- `070ea29` - `Normalize setup startup flow`

Step 5 commit:

- `12a4a46` - `Introduce standard screen navigation`

Step 6 commit:

- `d67a2f3` - `Replace runtime nav with BottomNavBar`

Step 7 commit:

- `feff0b2` - `Apply Cybershow theme foundation`

Step 8 commit:

- `ceb86a9` - `Redesign setup screen`

Step 9 commit:

- `22b51e9` - `Refactor control dashboard`

Step 10 commit:

- `fda477b` - `Refactor device traffic screen`

Step 11 is implemented in the working tree/next commit: Screen 3 Mapa / conexiones.

Current state:

- `cybershow_app_standards_v0_3/` is intentionally untracked and should remain untracked unless the operator decides otherwise.
- Codex should not compile or run the app; the operator handles compile, test, git validation, deploy, and release packaging.
- The next planned refactor step is Step 12: refactor Screen 4, Perfil de riesgo.

Important behavior already implemented:

- No arguments behave like `--configure`.
- `--configure` shows setup first.
- `--show` and `--design` skip setup and enter runtime directly.
- Setup can start runtime with Enter, Space, Right Arrow, `1`, or the primary button when focus is not editable.
- Runtime `Esc` requests setup only in configure launch mode.
- Runtime `Esc` is inert in show/design launch modes.
- Show/design setup lockout is currently enforced by startup flow; there is no setup path in those modes.
- Runtime navigation now uses screen definitions for the five standard screens.
- Number keys `1`-`5` and Left/Right navigate without wrapping.
- Letter shortcuts for primary navigation have been removed.
- Runtime uses one shared `BottomNavBar` at the window level.
- Bottom navigation clicks use the same screen-definition indices as keyboard navigation.
- The app now applies the shared `CyberTheme` stylesheet globally.
- Runtime uses `CyberBackgroundWidget` as the main window background.
- `ScreenPage` header/content frames use standard Cyber panel object names.
- Setup uses the common cyber background and a centered technical card.
- Setup no longer has a fixed side mascot image.
- Setup visible copy is now Spanish, with `INICIAR SHOW` as the primary action.
- Screen 1 is now titled `Centro de control Cybershow`.
- Screen 1 console headers are translated to Spanish.
- Screen 1 right panel now shows module mode, router state, local ports, device counts, and warnings instead of the mascot image.
- Screen 2 is now titled `Dispositivos + trafico`.
- Screen 2 panels are now `Dispositivos conectados / conocidos` and `Mensajes del router`.
- Screen 2 portal URL is presented as a deliberate operator info banner.
- Screen 2 credential reveal text is now `CREDENCIAL INTERCEPTADA`.
- Screen 2 raw traffic rows keep the same event pipeline while adding display-only service category labels and row color accents.
- Screen 3 is now titled `Mapa / conexiones`.
- Screen 3 selected-device header now uses `OBJETIVO` and the empty state is `Sin dispositivo seleccionado`.
- Screen 3 side panel is now `Eventos del dispositivo`.
- Screen 3 map projection has stronger contrast, larger destination labels, and a display-only destination pulse when packets arrive.

## Step 1: Document Public Wi-Fi App Spec

Status: completed.

Commit intent: create the app-specific contract before touching behavior.

Scope:

- Add root `APP_SPEC.md` based on the standards template.
- Define visible/internal names, executable, role, and mixed operative/scenic type.
- Document the five screens with standard numbers, ids, Spanish titles, short titles, and screen kinds.
- Document setup parameters, demo/live behavior, orchestrator messages, and justified exceptions.
- Keep `README.md`, `CLAUDE.md`, and this file aligned if the spec changes naming.

Acceptance:

- `APP_SPEC.md` exists and clearly describes Public Wi-Fi against the Cybershow standard.
- No runtime code changes.

Operator verification:

- Review docs only.

## Step 2: Import Shared Standard Components

Status: completed.

Commit intent: make the shared UI/common code available without changing app behavior.

Scope:

- Copy or integrate the required files from `cybershow_app_standards_v0_3/common` and `cybershow_app_standards_v0_3/ui` into the app source tree.
- Add the new files to `CMakeLists.txt`.
- Keep the existing UI active for now.
- Avoid moving application logic out of `MainWindow` in this step.

Acceptance:

- The project has local shared components available: CLI mode parser, screen definitions, orchestrator protocol, theme/background, bottom nav, panels, metrics, alerts.
- Existing app startup and screens remain unchanged.

Operator verification:

- Compile only.

## Step 3: Add Standard CLI Launch Modes

Status: completed.

Commit intent: implement `--configure`, `--design`, and `--show` startup behavior.

Scope:

- Extend startup configuration with `LaunchMode` while preserving current live/demo operating mode.
- Make no arguments equivalent to `--configure`.
- Support `--configure`, `--design`, `--show`, `--fullscreen`, `--windowed`, and reserve/profile-map `--profile demo/live/dev` where practical.
- Reject unknown or incompatible arguments with a clear error dialog and non-zero exit.
- In `--design` and `--show`, skip setup and enter execution directly using a safe default profile.

Acceptance:

- Launch mode and operating profile are separate concepts.
- Current setup selection still works in `--configure`.
- Show/design startup does not expose setup.

Operator verification:

- Compile.
- Manually launch with no args, `--configure`, `--show`, `--design`, and one invalid argument.

## Step 4: Normalize Setup Access And Startup Flow

Status: completed.

Commit intent: align setup behavior with the common operating model.

Scope:

- Allow `Esc` to return to setup only when launched in `--configure`.
- Block setup access completely in `--show` and `--design`.
- Add setup shortcuts: Enter, Space, Right Arrow, and `1` start execution when focus is not inside an editable field.
- Preserve existing Normal/Demo and Act Sequence choices.
- Keep router/demo startup side effects equivalent to the current app.

Acceptance:

- Setup is a technical preparation screen, not an exit path.
- Existing demo/live show flow still works.

Operator verification:

- Compile.
- Manually check setup keyboard behavior and show/design setup lockout.

## Step 5: Introduce Screen Definitions And Standard Navigation

Status: completed.

Commit intent: replace ad hoc navigation rules with screen metadata and common keyboard behavior.

Scope:

- Add `ScreenDefinition` metadata for all five pages:
  - `1 principal` / `Centro de control` / operative
  - `2 dispositivos` / `Dispositivos + trafico` / operative
  - `3 mapa` / `Mapa / conexiones` / operative
  - `4 riesgo` / `Perfil de riesgo` / operative
  - `5 cifrado` / `Analisis de cifrado` / scenic
- Map number keys `1`-`5` to the matching screen.
- Make Left/Right navigation non-circular.
- Remove letter shortcuts for primary navigation.
- Ensure shortcuts do not interfere with editable widgets.

Acceptance:

- Navigation numbers match the future bottom bar.
- Right arrow on screen 5 stays on screen 5; left arrow on screen 1 stays on screen 1.
- Existing screen content and event processing are not changed.

Operator verification:

- Compile.
- Manually check keyboard navigation.

## Step 6: Replace Bottom Navigation With Common BottomNavBar

Status: completed.

Commit intent: make navigation visually and behaviorally consistent with the Cybershow standard.

Scope:

- Replace the current bottom navigation/header implementation with the shared `BottomNavBar`.
- Use short Spanish labels: `Principal`, `Dispositivos`, `Mapa`, `Riesgo`, `Cifrado`.
- Highlight the active screen.
- Make clicks equivalent to number-key navigation.
- Keep the bar visible on all runtime screens, with a more discreet style on the scenic encryption screen if needed.

Acceptance:

- The bottom bar is fixed, numbered, clickable, and synchronized with keyboard navigation.
- No screen content logic changes.

Operator verification:

- Compile.
- Manually check bottom bar clicks and active state.

## Step 7: Apply Common Theme, Background, And Basic Components

Status: completed.

Commit intent: establish the common visual foundation before detailed screen fixes.

Scope:

- Apply `CyberTheme` and the common cyber background.
- Convert repeated dashboard containers to `CyberPanel` where this is low risk.
- Normalize base colors, panel borders, text colors, button colors, and font choices.
- Keep current widget structure unless a visual standard requires adjustment.
- Avoid changing router, TCP, portal, map, demo, or credential processing logic.

Acceptance:

- Runtime screens share the Cybershow dark technical visual base.
- Existing functionality remains intact.

Operator verification:

- Compile.
- Visual smoke test all five screens.

## Step 8: Redesign Setup Screen To Standard Template

Status: completed.

Commit intent: make setup match the common Cybershow preparation screen.

Scope:

- Redesign `InitScreen` as a centered technical setup card over the common background.
- Translate visible text to Spanish.
- Use title `Public Wi-Fi - Configuracion tecnica`.
- Remove the fixed side mascot/image from setup if present.
- Use a blue primary `INICIAR SHOW` button.
- Keep Normal/Demo and Act Sequence controls functionally equivalent.

Acceptance:

- Setup has the common card structure and no public/scenic elements.
- Existing configuration values are unchanged.

Operator verification:

- Compile.
- Manually check setup in fullscreen/windowed if applicable.

## Step 9: Refactor Screen 1, Centro De Control

Status: completed.

Commit intent: align the main control dashboard with the standard operative layout.

Scope:

- Translate title to `Centro de control Cybershow`.
- Replace the right-side mascot/identity block with a real status panel:
  - module name;
  - launch mode/profile;
  - router/live state;
  - server ports;
  - connected/known device count;
  - current warnings.
- Translate console panel titles:
  - `Nodo 01 // Vigilancia de dispositivos`
  - `Nodo 02 // Eventos de trafico`
  - `Nodo 03 // Logs del sistema`
  - `Nodo 04 // Conexiones activas`
- Preserve SSH console and router control behavior.

Acceptance:

- Screen 1 is an operative dashboard with useful status instead of decorative identity.
- Router script buttons and consoles keep their current behavior.

Operator verification:

- Compile.
- Manual live/demo smoke test for Screen 1.

## Step 10: Refactor Screen 2, Dispositivos + Trafico

Status: completed.

Commit intent: improve the device and raw traffic screen without changing capture logic.

Scope:

- Translate title to `Dispositivos + trafico`.
- Rename panels to `Dispositivos conectados / conocidos` and `Mensajes del router`.
- Restyle the portal URL band as a deliberate info banner.
- Translate `CREDENTIAL INTERCEPTED` to `CREDENCIAL INTERCEPTADA`.
- Normalize alert padding, hierarchy, and critical red styling.
- Color-code traffic rows by service category if this can be done as display-only formatting.

Acceptance:

- Credential capture behavior is unchanged.
- Traffic remains raw enough for operator trust, with clearer visual severity.

Operator verification:

- Compile.
- Manual demo credential event check.

## Step 11: Refactor Screen 3, Mapa / Conexiones

Status: completed.

Commit intent: align map copy and projection legibility while preserving packet visualization.

Scope:

- Translate title to `Mapa / conexiones`.
- Translate map labels such as `TARGET` to `OBJETIVO`.
- Rename side/event panel to `Eventos del dispositivo`.
- Review map contrast, label size, and panel spacing for projection readability.
- Change packet service label behavior to pulse at destination after arrival if this can be done without altering event routing.

Acceptance:

- Map still uses the existing region/service data and packet event pipeline.
- Destination feedback reads better on a projector.

Operator verification:

- Compile.
- Manual demo packet trail check.

## Step 12: Refactor Screen 4, Perfil De Riesgo

Commit intent: make the risk screen useful as an operative explanation surface.

Scope:

- Translate title to `Perfil de riesgo`.
- Preserve the current score/risk computation unless display adaptation requires a narrow change.
- Add useful lower-panel content:
  - target summary;
  - detected categories;
  - risk factors;
  - short timeline;
  - operator-facing explanation.
- Normalize status labels such as `CRITICO`, `LISTO`, `ADVERTENCIA`, and `ERROR`.

Acceptance:

- Existing service stats keep updating.
- The screen no longer feels empty below the main metric.

Operator verification:

- Compile.
- Manual demo traffic accumulation check.

## Step 13: Refactor Screen 5, Analisis De Cifrado

Commit intent: make the encryption scene match the standard while keeping the theatrical failure.

Scope:

- Translate title to `Analisis de cifrado`.
- Preserve the WhatsApp brute-force sequence and guaranteed failure.
- Keep the terminal aesthetic but improve readability.
- Translate or rename `Trigger Demo` to `Lanzar demo` or `Forzar evento`.
- Ensure visible copy makes the fictional/demonstrative nature clear where appropriate.
- Keep Act Sequence pause/resume behavior intact.

Acceptance:

- The encryption demo still ends in the current E2EE reassurance outcome.
- Screen reads as a scenic Cybershow screen.

Operator verification:

- Compile.
- Manual encryption sequence check.

## Step 14: Add Minimal Orchestrator Protocol

Commit intent: emit standard machine-readable runtime status without changing operation.

Scope:

- Emit `CYBERSHOW_STATUS READY` after successful initialization.
- Emit `CYBERSHOW_STATUS RUNNING` when entering execution.
- Emit `CYBERSHOW_SCREEN <n> <id>` on screen changes.
- Emit `CYBERSHOW_STATUS ERROR <code>` for relevant startup/resource failures where practical.
- Keep stdout reserved for `CYBERSHOW_*` messages if possible; send human diagnostics elsewhere.

Acceptance:

- The app can be supervised by the orchestrator using normalized stdout lines.
- Visual text is not used as protocol.

Operator verification:

- Compile.
- Launch from terminal and inspect stdout.

## Step 15: Add Logging And Configuration Follow-Up Hooks

Commit intent: prepare common logs/configuration without broad behavioral risk.

Scope:

- Add internal logging with common fields where useful:
  `timestamp | public-wifi | launchMode | profile | level | component | message`.
- Add or document `logs/public-wifi.log`.
- Document any postponed `--config` and persistent JSON configuration work.
- Confirm no real secrets are written to repo or logs.

Acceptance:

- Operational events have a stable log path/format or a documented exception.
- Remaining config work is explicit.

Operator verification:

- Compile.
- Manual log file inspection.

## Step 16: Final Standards Pass And Documentation Sync

Commit intent: close the refactor against the checklist and update project docs.

Scope:

- Review `CYBERSHOW_REFACTOR_CHECKLIST.md` against the implemented app.
- Update `README.md`, `CLAUDE.md`, and `APP_SPEC.md` with final startup commands, navigation, mode behavior, and known exceptions.
- Confirm Spanish screen naming is consistent across UI and docs.
- Remove dead visual helpers only if they are no longer referenced.
- Leave build, release, tag, and deploy actions to the operator.

Acceptance:

- Docs match the refactored application.
- Any standard exceptions are written down.
- No unrelated refactors are mixed in.

Operator verification:

- Compile.
- Full manual smoke test.
- Git commit/tag/release/deploy as needed.
