"""
EchoPath - Wireless Communication Module (WebSocket / SoftAP)
Handles all communication with the ESP32 over WiFi WebSocket (ws://192.168.4.1:81).

Protocol (ESP32 → Python):
  DIST:<cm>     — ultrasonic distance reading streamed over WebSocket
  CMD:<cmd>     — echo of the last command received by ESP32

Protocol (Python → ESP32):
  LEFT\n        — two short beeps
  RIGHT\n       — one long beep
  CENTER\n      — continuous tone / urgent beeps
  NONE\n        — buzzer off
"""

import threading
import time
from collections import deque
import websocket

# ---- CONFIG ----
WS_URL          = "ws://192.168.4.1:81"
RECONNECT_DELAY = 2.0       # seconds between reconnect attempts
MEDIAN_WINDOW   = 5         # rolling median window for distance smoothing

# ---- STATE ----
_latest_distance  = -1
_distance_history = deque(maxlen=MEDIAN_WINDOW)
_lock             = threading.Lock()
_ws               = None
_running          = True
_connected        = False


def _on_open(ws):
    global _connected
    with _lock:
        _connected = True
    print(f"[wireless_comm] Connected to ESP32 WebSocket @ {WS_URL}")


def _on_close(ws, close_status_code, close_msg):
    global _connected
    with _lock:
        _connected = False
    print("[wireless_comm] WebSocket connection closed.")


def _on_error(ws, error):
    print(f"[wireless_comm] WebSocket error: {error}")


def _on_message(ws, message):
    global _latest_distance
    raw = str(message).strip()

    if raw.startswith("DIST:"):
        try:
            value = int(raw.split(":")[1])
            with _lock:
                if value > 0:           # ignore -1 (sensor timeout)
                    _distance_history.append(value)
                _latest_distance = value
        except ValueError:
            pass    # malformed line — skip silently

    elif raw.startswith("CMD:"):
        print(f"[wireless_comm] ESP32 ACK  → {raw}")

    elif raw == "EchoPath Ready":
        print("[wireless_comm] ESP32 booted and ready.")


def _ws_loop():
    """Background thread: connects and maintains WebSocket connection.
    Automatically reconnects if the connection drops."""
    global _ws, _connected, _running

    while _running:
        try:
            print(f"[wireless_comm] Connecting to {WS_URL}...")
            ws = websocket.WebSocketApp(
                WS_URL,
                on_open=_on_open,
                on_message=_on_message,
                on_error=_on_error,
                on_close=_on_close,
            )
            with _lock:
                _ws = ws
            ws.run_forever(ping_interval=10, ping_timeout=5)
        except Exception as e:
            print(f"[wireless_comm] WebSocket loop error: {e}")

        with _lock:
            _connected = False
            _ws = None

        if _running:
            print(f"[wireless_comm] Connection lost. Retrying in {RECONNECT_DELAY}s...")
            time.sleep(RECONNECT_DELAY)


# ── Public API ────────────────────────────────────────────────────────────────

def is_connected() -> bool:
    """Returns True if the WebSocket connection is active."""
    with _lock:
        return _connected and _ws is not None


def get_distance() -> int:
    """Returns the median-smoothed distance in cm, or -1 if no valid reading."""
    with _lock:
        if not _distance_history:
            return -1
        return sorted(_distance_history)[len(_distance_history) // 2]


def send_command(command: str):
    """Send a direction command (LEFT / RIGHT / CENTER / NONE) over WebSocket.
    The ESP32 firmware translates these into distinct buzzer patterns."""
    with _lock:
        if not _connected or _ws is None:
            return
        try:
            _ws.send(command + "\n")
            print(f"[wireless_comm] {time.strftime('%H:%M:%S')} SENT  {command}")
        except Exception as e:
            print(f"[wireless_comm] Failed to send command '{command}': {e}")


def shutdown():
    """Call on program exit to cleanly close the WebSocket connection."""
    global _running, _connected
    _running   = False
    _connected = False
    time.sleep(0.15)
    with _lock:
        if _ws:
            try:
                _ws.close()
                print("[wireless_comm] WebSocket closed.")
            except Exception:
                pass


# ── Start background reader immediately on import ─────────────────────────────
_thread = threading.Thread(target=_ws_loop, daemon=True)
_thread.start()
