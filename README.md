
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

A — Intro / start screen  
B — Devices + raw router traffic  
C — Detailed view of selected device  
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
- CMake
- MSVC (Windows) or Clang/GCC (Linux)

Example build:

mkdir build
cd build
cmake ..
cmake --build .

Typical Qt installation path on Windows:

C:/Qt/6.x/msvc2022_64

---

# Router Setup

Router used during development:

GL.iNet GL‑MT300N‑V2 (Mango) running OpenWrt.

Connect to the router:

ssh root@192.168.8.1

---

# Router Scripts

Traffic events script

/root/send_traffic_events.sh <host> <port>

Example:

/root/send_traffic_events.sh 192.168.8.182 5555

This script reads DNS logs and converts them into JSON traffic events.

Example output:

{"ts":1773062293,"device":"iPhone","ip":"192.168.8.185","event":"SEARCH","domain":"google.com"}

---

Device connection script

/root/device_watch.sh <host> <port>

Example:

/root/device_watch.sh 192.168.8.182 5556

This monitors Wi‑Fi association and reports when devices connect or disconnect.

Example:

{"type":"device","action":"connected","device":"iPhone","ip":"192.168.8.185"}

---

# Running the Full System

1. Start the Qt application

2. Connect to the router via SSH

3. Start both router scripts (two terminals recommended)

/root/send_traffic_events.sh 192.168.8.182 5555

/root/device_watch.sh 192.168.8.182 5556

4. Connect the demonstration phone to the router Wi‑Fi

5. Start browsing (Google, Amazon, WhatsApp, etc.)

Events will appear live in the UI.

---

# Ethical Constraints

This demonstration:

- never inspects audience devices
- uses a dedicated private router
- only monitors a controlled phone used for the show

The purpose is **education and awareness**, not intrusion.

---

# Current Status

Working prototype:

- Qt application running
- Router scripts producing events
- Live traffic displayed
- Device connection detection implemented
- Multi‑screen navigation operational

Next milestone:

Map visualization of traffic connections.

