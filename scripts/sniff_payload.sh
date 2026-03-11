#!/bin/sh

TARGET_IP="$1"

if [ -z "$TARGET_IP" ]; then
  echo "Usage: $0 <target_ip>"
  exit 1
fi

# Filter for HTTPS (443) or WhatsApp (5222) traffic from the target IP
# -c 5: Capture only 5 packets
# -X: Print hex and ASCII
# -n: Don't resolve names
# -q: Quiet output
# -i any: Listen on all interfaces

# We want a concise hex dump. tcpdump -c 1 -X will show one packet.
# We'll grab just enough to look "technical".

tcpdump -i any -c 1 -X -n -q host "$TARGET_IP" and \(port 443 or port 5222\) 2>/dev/null | grep '^  0x' | head -n 20
