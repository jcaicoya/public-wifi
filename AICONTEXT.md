
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
Qt Widgets  
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

# Router Scripts

Traffic script

/root/send_traffic_events.sh

Reads dnsmasq logs and emits JSON events such as:

{
 "ts":1773062293,
 "device":"iPhone",
 "ip":"192.168.8.185",
 "event":"SEARCH",
 "domain":"google.com"
}

Device script

/root/device_watch.sh

Uses:

iwinfo ra0 assoclist

and

/tmp/dhcp.leases

to detect Wi‑Fi connections.

Example JSON output:

{
 "type":"device",
 "action":"connected",
 "device":"iPhone",
 "ip":"192.168.8.185"
}

---

# Qt Application Structure

The application currently contains five screens:

A – Intro  
B – Devices + raw router messages  
C – Device detail view  
D – Statistics view  
E – Encryption demonstration

Navigation shortcuts:

1 → A  
2 → B  
3 → C  
4 → D  
5 → E

Screen B shows:

- device list
- raw router JSON messages

Screen C shows:

- traffic from the selected device

---

# Current Implementation Status

Working

- TCP servers implemented
- Router scripts functioning
- Live JSON events received
- Device detection working
- Multi‑screen navigation implemented

Partially implemented

- device list population logic
- device selection filtering

Not implemented yet

- map visualization
- animated connections
- statistics dashboard
- encryption demonstration logic

---

# Planned Features

Visualization

Display a world or Europe map showing:

phone → remote services

Animated connections.

Event classification

Current detected services:

SEARCH  
AMAZON  
WHATSAPP  
VIDEO  
APPLE  
MICROSOFT

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

1. Implement map visualization on screen C
2. Aggregate traffic events for statistics screen
3. Improve router scripts (persistent connection)
4. Design visual animations for traffic flows

