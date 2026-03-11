#!/bin/sh

HOST="$1"
PORT="$2"

if [ -z "$HOST" ] || [ -z "$PORT" ]; then
  echo "Usage: $0 <host> <port>"
  exit 1
fi

STATE_FILE="/tmp/cybershow_wifi_state.txt"
touch "$STATE_FILE"

get_connected_macs() {
    iwinfo ra0 assoclist \
    | awk '
        /^[0-9A-Fa-f:]{17}[[:space:]]/ {
            mac=toupper($1)
            if (mac != "00:00:00:00:00:00")
                print mac
        }
    ' \
    | sort -u
}

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

send_event() {
    ACTION="$1"
    NAME="$2"
    IP="$3"

    [ -z "$NAME" ] && NAME="$IP"

    JSON_MSG="{\"type\":\"device\",\"action\":\"$ACTION\",\"device\":\"$NAME\",\"ip\":\"$IP\"}"
    
    # Print to console for SSH visibility
    echo "$JSON_MSG" >&2
    
    # Send to TCP server. Using a short timeout (-w 2) to not block if server is down
    echo "$JSON_MSG" | nc -w 2 "$HOST" "$PORT"
}

echo "Starting device watch to $HOST:$PORT..." >&2

while true
do
    NEW="$(get_connected_macs)"
    OLD="$(cat "$STATE_FILE" 2>/dev/null)"

    # New connections
    for mac in $NEW; do
        echo "$OLD" | grep -Fxq "$mac"
        if [ $? -ne 0 ]; then
            DEVICE="$(resolve_device "$mac")"
            NAME="$(echo "$DEVICE" | cut -d'|' -f1)"
            IP="$(echo "$DEVICE" | cut -d'|' -f2)"
            send_event "connected" "$NAME" "$IP"
        fi
    done

    # Disconnections
    for mac in $OLD; do
        echo "$NEW" | grep -Fxq "$mac"
        if [ $? -ne 0 ]; then
            DEVICE="$(resolve_device "$mac")"
            NAME="$(echo "$DEVICE" | cut -d'|' -f1)"
            IP="$(echo "$DEVICE" | cut -d'|' -f2)"
            send_event "disconnected" "$NAME" "$IP"
        fi
    done

    printf '%s\n' "$NEW" > "$STATE_FILE"
    sleep 2
done