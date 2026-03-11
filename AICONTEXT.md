# AI Context – Public Wi‑Fi Cybershow

This file summarizes the project context so that a new AI session can continue development quickly.

---

# Project Overview

The project is an educational theatrical demonstration about public Wi‑Fi privacy risks.

A controlled phone connects to a controlled Wi‑Fi network and the audience sees a visual representation of network activity.

The goal is:

- educational
- ethical
- understandable to non‑technical audiences
- visually engaging

No real attacks are performed.

---

# Technology Stack

Application

C++  
Qt 6  
Qt Widgets (QPainter, QGraphicsView concepts)
CMake

Development environment

Windows 11  
CLion  
MSVC

Router

GL.iNet GL‑MT300N‑V2 (Mango)  
OpenWrt

---

# System Architecture

Phone
  │
  ▼
Router (OpenWrt)
  │
  ├─ DNS logs
  ├─ Wi‑Fi association state
  │
  ▼
Router scripts
  │
  ▼
JSON events (TCP)
  │
  ▼
Qt Application

Ports used:

5555 → traffic events  
5556 → device events

---

# Qt Application Structure

The application currently contains five screens:

A – Main (Auto-connects to router + Cuarzito mascot)
B – Devices + raw router messages  
C – Navigation (Map Visualization and device event detail view)
D – Statistics view  
E – Encryption demonstration

Navigation shortcuts:

1 / M → A  
2 / D → B  
3 / N → C  
4 / S → D  
5 / E → E
Left/Right Arrows -> Cycle Screens

---

# Current Implementation Status

Working

- TCP servers implemented
- JSON parsing extracted into core `processTrafficEvent` and `processDeviceEvent`
- Router scripts functioning (device watch and traffic events)
- **Project Structure Refactored:** Code moved to `src/`, static assets to `resources/`
- **Auto-connect & Background Management:** Application automatically connects to the router on startup, runs SSH commands securely in `QProcess`, and guarantees zombie processes are killed on exit.
- **Demo Mode CLI Parameter:** Demo mode is now toggled via `--demo` command line argument.
- Multi‑screen navigation operational with global letter/number and arrow key `QShortcut`s
- Device list population and selection UI auto-navigation
- Custom `MapView` widget drawing a low-poly geometric world map using `QPainterPath`
- Animated traffic connections from fixed origin (Asturias, Spain) to remote geographic regions (North America, Europe, Asia, etc.)
- Visual region highlighting on packet arrival
- Map packet particle trails, auto-scrolling log, and global dark mode
- Statistics dashboard logic and data aggregation (Screen D)
- Encryption demonstration logic simulating the attempt and fail (Screen E)

Not implemented yet

- Event classification enhancements (mapping raw domains to distinct services)
- Deep packet inspection on router (exploring sniffing limits)

---

# Planned Features

Event classification

Current detected services mapped to regions:

SEARCH -> North America
AMAZON -> Europe
WHATSAPP -> North Europe
VIDEO -> Asia
APPLE -> North America
MICROSOFT -> North America

Future additions may include TikTok, Instagram, etc.

Encryption demonstration

At a key moment in the show:

- WhatsApp traffic is detected
- the system attempts to decode it
- decoding fails
- the audience sees that encryption protects the content

---

# Key Design Principle

The show must demonstrate:

- networks see metadata
- encrypted apps protect content
- public Wi‑Fi is not neutral

without performing any real intrusion.

---

# Recommended Next Development Steps

1. **Focus on Encryption Screen:** Explore the limits of the OpenWrt router and its sniffing capabilities (can we capture unencrypted HTTP payloads to demonstrate a successful MITM?).
2. **Improve World Map:** Refine the low-poly SVG coordinates, add more geographic regions, or introduce pulsing animations for active data centers.
3. **Improve Events Mapping:** Expand the domain-to-service translation logic for a more readable and impactful audience experience.
4. **Improve UI:** General aesthetic polish, smoother transitions between screens, and font/layout adjustments for maximum theatrical readability.
5. **Stability & Performance (Technical):** Implement robust reconnection logic for the TCP servers if the router drops off mid-show, and optimize `MapView` repaints to reduce CPU load during heavy traffic spikes.
6. (Optional) Integrate sound effects for device connections
