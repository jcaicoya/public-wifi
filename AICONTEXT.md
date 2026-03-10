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

A – Intro (Controls for Router & Demo Mode)
B – Devices + raw router messages  
C – Map Visualization and device event detail view
D – Statistics view  
E – Encryption demonstration

Navigation shortcuts:

1 → A  
2 → B  
3 → C  
4 → D  
5 → E

---

# Current Implementation Status

Working

- TCP servers implemented
- JSON parsing extracted into core `processTrafficEvent` and `processDeviceEvent`
- Router scripts functioning
- Remote Router Control (SSH background commands via `QProcess`)
- Virtual Router / Demo Mode (`QTimer` injecting events from `demo_events.json`)
- Device list population and selection UI auto-navigation
- Custom `MapView` widget drawing a low-poly geometric world map using `QPainterPath`
- Animated traffic connections from fixed origin (Asturias, Spain) to remote geographic regions (North America, Europe, Asia, etc.)
- Visual region highlighting on packet arrival

Not implemented yet

- statistics dashboard logic (data aggregation)
- encryption demonstration logic (simulating the attempt and fail)

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

1. Aggregate traffic events to build the visual Statistics Dashboard (Screen D)
2. Implement the Encryption Demo logic to show "locked" traffic (Screen E)
3. Improve router scripts (ensure persistent connection and reliability)
