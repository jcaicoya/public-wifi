# IMPROVEMENTS — Reusable Qt UI Capabilities

This document describes reusable capabilities that should be implemented in Cybershow Qt apps the same way across projects:

1. a runtime mode badge that shows `DEMO` or `LIVE`
2. a bottom navigation bar that can be shown or hidden during the show
3. a common launch mode contract so all apps start the same way

The goal is consistency. If another Qt app reads this document, it should be able to implement both features with the same shortcuts, the same blinking behavior, the same colors, and the same operator expectations.

The same applies to launch modes:

- no arguments must behave like `--configure`
- `--configure` opens Setup
- `--demo` skips Setup and starts in demo mode
- `--live` skips Setup and starts in live mode
- launch-mode names, startup behavior, and operator expectations must match across apps

If an app has a Setup screen, it should use the same launch contract and the same direct-start behavior as the other Cybershow Qt apps.

---

## 1. Runtime Mode Badge

### Purpose

The badge tells the operator whether the app is running in demo mode or live mode. It is only an operator aid. It should not be a decorative element, and it should never steal focus or capture clicks.

### Exact Text

- Demo mode: `DEMO`
- Live mode: `LIVE`

### Default State

- hidden by default
- not visible to the audience until the operator enables it

### Toggle Shortcut

- `F10` toggles the badge on and off

### Visual Rules

- position: center-right of the main window
- alignment: centered inside its bounding box
- text color: white
- background: transparent
- font family: `Consolas`, `monospace`
- font size: `64px`
- font weight: `900`
- letter spacing: `8px`

### Blink / Pulse Behavior

The badge should pulse continuously using opacity animation:

- start opacity: `0.18`
- peak opacity: `0.92`
- end opacity: `0.18`
- duration: `2500 ms`
- easing: `InOutSine`
- loop: infinite

The animation should continue while the badge exists, even if it is hidden, so showing it again feels immediate.

### Interaction Rules

- no mouse interaction
- no keyboard focus
- no text selection
- no text editing

Recommended Qt settings:

- `setFocusPolicy(Qt::NoFocus)`
- `setAttribute(Qt::WA_TransparentForMouseEvents, true)`
- `setTextInteractionFlags(Qt::NoTextInteraction)`

### Implementation Pattern

Use a `QLabel` parented to the main window, not to one screen widget. That keeps the badge visible across screen changes.

Recommended structure:

- `QLabel` for text
- `QGraphicsOpacityEffect` for pulse
- `QPropertyAnimation` on the `opacity` property
- helper function to update the text depending on the current operating mode
- helper function to show/hide the badge

### Expected Behavior By Mode

- In `demo`, the badge text is `DEMO`
- In `live`, the badge text is `LIVE`
- The visibility state is still controlled by `F10`

The badge should not appear automatically just because the app is in demo or live mode. The operator decides whether it is visible.

---

## 2. Bottom Navigation Bar Visibility

### Purpose

The app should keep keyboard navigation active but allow the operator to hide the visible navigation buttons during the public show.

The public should be able to experience the show without the control strip on screen, while the operator still navigates with numbers and arrows.

### Default State

- hidden by default
- keyboard navigation remains active even when hidden

### Toggle Shortcut

- `F9` toggles the navigation bar on and off

### What It Controls

The visibility toggle applies to the bottom button row only.

It does not disable:

- number keys `1` to `9`
- left/right arrow navigation
- screen selection logic
- screen change protocol messages

### Interaction Rules

The navigation bar itself should still behave like a normal Cybershow bottom bar when visible:

- clickable
- current screen highlighted
- disabled screens shown dimmed if applicable

When hidden, it should be removed from view without breaking layout or navigation.

### Implementation Pattern

Use the existing bottom navigation widget and hide/show the widget itself.

Recommended structure:

- parent layout owns the bar
- main window stores a boolean visible state
- `F9` flips that boolean
- a helper function applies `setVisible(true/false)` on the bar

### Operator Expectation

The operator should be able to:

- hide the bar before or during the show
- show it again for rehearsal or troubleshooting
- keep full keyboard control regardless of the bar visibility

---

## 3. Shared Shortcuts

These shortcuts should be reused across Qt apps that adopt this standard:

- `F9` = show/hide bottom navigation bar
- `F10` = show/hide runtime mode badge
- `1` to `9` = direct screen navigation
- `Left Arrow` / `Right Arrow` = previous / next screen

These shortcuts should not interfere with editable fields.

If a text input, text editor, combo box, or spin box has focus, global navigation shortcuts should not override normal editing behavior.

---

## 4. Shared Colors

The badge and bar visibility should fit the existing Cybershow palette:

- white text for the badge
- transparent badge background
- dark technical UI background for the app
- blue/cyan accents for active UI elements
- dimmed or hidden navigation when not in use

Do not introduce a different accent system just for these capabilities.

---

## 5. Acceptance Criteria

A Qt app should be considered compliant when:

- `DEMO` / `LIVE` badge exists
- badge is non-interactive
- badge is hidden by default
- badge toggles with `F10`
- bottom navigation bar is hidden by default
- bottom navigation bar toggles with `F9`
- keyboard navigation still works while the bar is hidden
- the visual treatment matches the Cybershow style

---

## 6. Notes For Future Refactors

If you need to port these capabilities to another Qt app, implement them in the same order:

1. add the mode badge overlay
2. add the `F10` toggle
3. hide the bottom navigation bar by default
4. add the `F9` toggle
5. verify that screen navigation still works with keyboard only
6. verify that editable widgets still receive normal typing behavior
7. align the launch modes with `--configure`, `--demo`, and `--live`
8. verify that no arguments still maps to `--configure`

The point is not just to copy behavior. The point is to keep the operator model identical across the Cybershow Qt family.
