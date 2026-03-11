#!/bin/sh

HOST="$1"
PORT="$2"

if [ -z "$HOST" ] || [ -z "$PORT" ]; then
  echo "Usage: $0 <host> <port>"
  exit 1
fi

STATE_FILE="/tmp/cybershow_wifi_state.txt"
# Ensure state file exists
touch "$STATE_FILE"

# Function to get current MACs from Wi-Fi driver
get_connected_macs() {
    iwinfo ra0 assoclist \
    | awk '
        /^[0-9A-Fa-f:]{17}[[:space:]]/ {
            mac=toupper($1)
            if (mac != "00:00:00:00:00:00")
                print mac
        }
    ' | sort -u
}

# Function to get IP and Hostname from DHCP leases
resolve_device() {
    MAC="$1"
    awk -v mac="$MAC" '
        BEGIN { IGNORECASE=1 }
        toupper($2) == toupper(mac) {
            ip=$3
            name=$4
            if (name == "" || name == "*") name=ip
            printf "%s|%s\n", name, ip
            exit
        }
    ' /tmp/dhcp.leases
}

# Function to send JSON to Qt App
send_event() {
    ACTION="$1"
    NAME="$2"
    IP="$3"

    # Don't send completely empty events
    if [ -z "$NAME" ] && [ -z "$IP" ]; then
        return
    fi

    [ -z "$NAME" ] && NAME="$IP"

    JSON_MSG="{\"type\":\"device\",\"action\":\"$ACTION\",\"device\":\"$NAME\",\"ip\":\"$IP\"}"

    # Mirror to stderr so it shows up in Qt Node 01 Console
    echo "$JSON_MSG" >&2

    # Send via TCP. We use a group { ... } to pipe multiple commands to nc.
    # We add a trailing newline explicitly and a small sleep (0.2s).
    # This keeps the TCP connection open long enough for the Qt event loop to read it.
    { echo "$JSON_MSG"; sleep 0.2; } | nc "$HOST" "$PORT" >/dev/null 2>&1
}
echo "Starting device watch to $HOST:$PORT..." >&2

# --- NEW: Announce already connected devices on startup ---
> "$STATE_FILE" # Ensure it starts empty
CURRENT_MACS="$(get_connected_macs)"
for mac in $CURRENT_MACS; do
    # Skip empty lines
    [ -z "$mac" ] && continue

    DEVICE="$(resolve_device "$mac")"
    if [ -n "$DEVICE" ]; then
        NAME="$(echo "$DEVICE" | cut -d'|' -f1)"
        IP="$(echo "$DEVICE" | cut -d'|' -f2)"
        send_event "connected" "$NAME" "$IP"
        # Only save to state file if we successfully sent the connected event
        echo "$mac" >> "$STATE_FILE"
    else
        echo "Startup: Waiting for DHCP lease for $mac..." >&2
    fi
done

# --- Main Watch Loop ---
while true
do
    NEW_MACS="$(get_connected_macs)"
    OLD_MACS="$(cat "$STATE_FILE" 2>/dev/null)"

    NEXT_STATE_FILE="/tmp/cybershow_wifi_state_next.txt"
    > "$NEXT_STATE_FILE"

    # 1. Check for New Connections
    for mac in $NEW_MACS; do
        [ -z "$mac" ] && continue

        # Is this MAC missing from the OLD list?
        if ! echo "$OLD_MACS" | grep -Fxq "$mac"; then

            # It's a new connection. Try to resolve DHCP.
            DEVICE="$(resolve_device "$mac")"

            if [ -n "$DEVICE" ]; then
                # DHCP is ready! Send event and add to state.
                NAME="$(echo "$DEVICE" | cut -d'|' -f1)"
                IP="$(echo "$DEVICE" | cut -d'|' -f2)"
                send_event "connected" "$NAME" "$IP"
                echo "$mac" >> "$NEXT_STATE_FILE"
            else
                # DHCP isn't ready. Don't add to state file so we check again next loop.
                echo "Waiting for DHCP lease for $mac..." >&2
            fi
        else
            # We already know about this MAC. Keep it in the state.
            echo "$mac" >> "$NEXT_STATE_FILE"
        fi
    done

    # 2. Check for Disconnections
    for mac in $OLD_MACS; do
        [ -z "$mac" ] && continue

        # Is this OLD MAC missing from the NEW list?
        if ! echo "$NEW_MACS" | grep -Fxq "$mac"; then
            DEVICE="$(resolve_device "$mac")"
            if [ -n "$DEVICE" ]; then
                NAME="$(echo "$DEVICE" | cut -d'|' -f1)"
                IP="$(echo "$DEVICE" | cut -d'|' -f2)"
                send_event "disconnected" "$NAME" "$IP"
            else
                send_event "disconnected" "$mac" "unknown"
            fi
        fi
    done

    # Overwrite old state with the new state, stripping any blank lines
    sed '/^$/d' "$NEXT_STATE_FILE" > "$STATE_FILE"

    sleep 2
done