# UI & Presentation Design - Public Wi-Fi Cybershow

This document outlines the visual aesthetic, color palette, and the intended theatrical flow of the application during a live show.

---

## 1. Visual Aesthetic ("Cyber" Theme)

The application is designed to look like a high-tech, slightly intimidating "hacker" or "analyst" dashboard. The goal is to visually impress the audience while keeping the data easy to read from a distance.

### Color Palette

*   **Backgrounds:**
    *   Deep Cyber Background: `#090C10` (used in MapView and primary windows)
    *   List/Console Background: `#202020`
*   **Foreground / Text:**
    *   Primary Text: White (`#FFFFFF`) or Light Gray
    *   Subdued/Grid Lines: Dim Blue-Gray (`rgba(30, 40, 60, 150)`)
*   **Action Colors:**
    *   **Target Phone:** Cyber Green (`#00FF44`) - Represents the device being monitored.
    *   **Data Packets / Lines:** Cyan (`#00FFFF`) - Represents active data moving across the network.
    *   **Alert / Warning (Future):** Red (`#FF3333`) - To be used for insecure connections or failed encryption attempts.
    *   **Region Base Color:** Muted Dark Blue (`rgba(20, 30, 50, 100)`)
    *   **Region Edge Color:** Slate Blue (`rgba(50, 80, 120, 150)`)

### Typography
*   **Monospace / Terminal Fonts:** `Consolas` (or similar, like `Courier New` or `Fira Code`) should be prioritized for labels, IPs, and logs to maintain the "terminal" aesthetic.
*   **Weights:** Heavy/Bold fonts are used for region labels and main titles to ensure legibility on projectors.

---

## 2. The Theatrical Flow (Show Script)

This outlines how the presenter interacts with the Qt Application during the stage demonstration.

### Step 1: The Setup (Screen A)
*   **Action:** Presenter welcomes the audience and explains the premise (the danger of open Wi-Fi).
*   **UI Interaction:** Presenter clicks **"Start Router Scripts"** (or "Demo Mode" if practicing).
*   **Visual:** The audience sees the initial Cybershow title screen.

### Step 2: The Trap (Screen B - Devices & Raw Logs)
*   **Action:** Presenter asks a volunteer to connect their phone to the show's Wi-Fi network (or uses their own test phone).
*   **UI Interaction:** Presenter presses `2` to navigate to **Screen B**.
*   **Visual:** The audience sees the raw, unreadable JSON data starting to scroll rapidly on the right. On the left, a new device (`iPhone`, `Android`, or `DemoPhone`) pops into the list highlighted in **Green**, revealing its internal IP address.
*   **Narrative:** "You are connected. You think you are safe, but look at what the network sees."

### Step 3: The Reveal (Screen C - The Map)
*   **Action:** The presenter wants to show exactly *what* the phone is doing.
*   **UI Interaction:** Presenter clicks the target phone in the list on Screen B, instantly jumping to **Screen C**.
*   **Visual:** The Map View. The audience sees the phone pulsating in Green in Asturias, Spain. As the user opens apps (WhatsApp, Amazon, Netflix), bright Cyan packets shoot across the map, illuminating entire continents (North Europe, America, Asia).
*   **Narrative:** "Every time you open an app, you are broadcasting your destination to anyone listening. You just opened WhatsApp; the data is flying to North Europe."

### Step 4: The Profile (Screen D - Statistics)
*   **Action:** The presenter summarizes the user's behavior.
*   **UI Interaction:** Presenter presses `4` to navigate to **Screen D**.
*   **Visual:** Aggregated stats. "You spent 45 seconds on Amazon and made 12 requests to Facebook."
*   **Narrative:** "We don't just see individual packets; we can build a profile of your habits."

### Step 5: The Defense (Screen E - Encryption)
*   **Action:** The presenter explains how we protect ourselves. They attempt to "read" the WhatsApp messages.
*   **UI Interaction:** Presenter presses `5` to navigate to **Screen E**.
*   **Visual:** A simulated hacking attempt that results in a giant, secure "ENCRYPTED / LOCKED" padlock screen.
*   **Narrative:** "We can see *where* you go, but thanks to end-to-end encryption, the actual content of your messages is locked away from the network."

---

## 3. UI/UX Enhancements

**Completed Enhancements:**
*   **Particle Trails:** Added fading tails to the moving packets on the map.
*   **Dark Mode Override:** Ensured the entire Qt application window frame respects the dark theme (custom styling for `QStackedWidget` panes).
*   **Live Event Ticker:** On the Map screen, the text log of events auto-scrolls to the bottom as new packets fly across the map.

**Future Enhancements:**
*   **Sound Effects:** Consider adding subtle "blip" or "typing" sound effects using `QSoundEffect` when new devices connect.