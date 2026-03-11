# Public Wi-Fi Cybershow

Educational / theatrical demonstration of public Wi-Fi privacy risks.

This project powers a live stage demonstration where a phone connects to a controlled Wi‑Fi network and the audience can see network activity visualized in real time.

The goal is **awareness, not hacking**.  
All traffic shown is from a **controlled device and a private network created for the show**.

---

# Architecture

Phone / Tablet
      │
      ▼
GL.iNet Router (OpenWrt)
      │
      │ DNS logs
      ▼
Router scripts
      │
      │ JSON events (TCP)
      ▼
Qt Application
      │
      ▼
Visual presentation for the audience

Two kinds of events are generated:

Traffic events:
SEARCH, AMAZON, WHATSAPP, MICROSOFT, APPLE, VIDEO

Device events:
connected, disconnected

---

# Qt Application

The application uses **Qt Widgets** and contains five screens.

A — Intro / start screen (Controls router scripts & Demo Mode)
B — Devices + raw router traffic  
C — Detailed map visualization of selected device  
D — Statistics / historical view  
E — Encryption demonstration (WhatsApp)

Keyboard shortcuts:

1 → Screen A  
2 → Screen B  
3 → Screen C  
4 → Screen D  
5 → Screen E

---

# Building the Application

Requirements

- Qt 6
- CMake (3.28+)
- MSVC (Windows) or Clang/GCC (Linux)

Example build:

mkdir build
cd build
cmake ..
cmake --build .

---

# Router Setup

Router used during development:

GL.iNet GL‑MT300N‑V2 (Mango) running OpenWrt.

Connect to the router:

ssh root@192.168.8.1

### Remote Router Control

The Qt Application can automatically start and stop the router scripts via SSH in the background. To enable this, SSH keys must be set up so the PC can access the router without a password prompt.

---

# Demo Mode (Virtual Router)

The application includes a built-in "Demo Mode" that acts as a Virtual Router. When activated from Screen A, it parses an embedded `demo_events.json` file and injects events into the core processing pipeline every 1.5 seconds. 

This completely simulates a live show environment without needing to connect a physical phone or router.

---

# Ethical Constraints

This demonstration:

- never inspects audience devices
- uses a dedicated private router
- only monitors a controlled phone used for the show

The purpose is **education and awareness**, not intrusion.

---

# Current Status

Working features:

- Qt application running
- TCP servers and JSON parsing implemented
- Router scripts producing events
- Live traffic displayed in raw logs
- Device connection detection and filtering implemented
- Multi‑screen navigation operational
- **Remote SSH control of router scripts**
- **Dynamic low-poly map visualization with geographic regions**
- **Animated data packet flows and region highlights (with particle trails)**
- **Virtual Router (Demo Mode) for testing**
- **Statistics Dashboard (Screen D) aggregating data in real time**
- **Encryption Demonstration sequence (Screen E) with terminal and locked screen**
- **Global dark mode UI applied**

Next milestones:

1. Improve router scripts (ensure persistent connection and reliability).
2. (Optional) Integrate sound effects for device connections.