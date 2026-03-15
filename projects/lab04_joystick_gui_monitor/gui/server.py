import argparse
import asyncio
import json
import sys
from typing import Optional

import serial
import websockets


# -------- Parsing helpers --------
def parse_line(line: str) -> Optional[dict]:
    """
    Accepts either:
      1) JSON: {"x":-1,"y":0,"mode":1,"led":{"L":1,"R":0,"U":1,"D":0}}
      2) CSV:  x,y,mode   (example: -1,0,1)
      3) Key=Val: x=-1 y=0 mode=1
    Returns dict or None if invalid.
    """
    s = line.strip()
    if not s:
        return None

    # JSON
    if s.startswith("{") and s.endswith("}"):
        try:
            obj = json.loads(s)
            if isinstance(obj, dict):
                return obj
        except json.JSONDecodeError:
            return None

    # CSV: -1,0,1
    if "," in s:
        parts = [p.strip() for p in s.split(",")]
        if len(parts) >= 3:
            try:
                x = int(parts[0])
                y = int(parts[1])
                mode = int(parts[2])
                return {"x": x, "y": y, "mode": mode}
            except ValueError:
                pass

    # key=val: x=-1 y=0 mode=1
    tokens = s.replace(";", " ").split()
    out = {}
    for t in tokens:
        if "=" in t:
            k, v = t.split("=", 1)
            k = k.strip()
            v = v.strip()
            if k in ("x", "y", "mode"):
                try:
                    out[k] = int(v)
                except ValueError:
                    return None
    if "x" in out and "y" in out and "mode" in out:
        return out

    return None


# -------- WebSocket server --------
CLIENTS = set()

async def ws_handler(websocket):
    CLIENTS.add(websocket)
    try:
        await websocket.wait_closed()
    finally:
        CLIENTS.discard(websocket)

async def broadcast(obj: dict):
    if not CLIENTS:
        return
    msg = json.dumps(obj, separators=(",", ":"))
    dead = []
    for ws in CLIENTS:
        try:
            await ws.send(msg)
        except Exception:
            dead.append(ws)
    for ws in dead:
        CLIENTS.discard(ws)

async def serial_reader(port: str, baud: int):
    try:
        ser = serial.Serial(port, baudrate=baud, timeout=0.2)
    except Exception as e:
        print(f"[!] Could not open serial port {port}: {e}", file=sys.stderr)
        print("[!] Tip: Check COM port, permissions, and that nothing else is using it.", file=sys.stderr)
        raise

    print(f"[*] Reading serial from {port} @ {baud}...")

    # Simple “last known state” so UI doesn’t go stale
    last = {"x": 0, "y": 0, "mode": 0}

    loop = asyncio.get_event_loop()

    while True:
        try:
            # Run blocking readline() in a thread so the asyncio event loop
            # stays free to accept WebSocket connections while waiting for data.
            raw_bytes = await loop.run_in_executor(None, ser.readline)
            raw = raw_bytes.decode("utf-8", errors="ignore")
        except Exception:
            raw = ""
            await asyncio.sleep(0.01)
            continue

        obj = parse_line(raw)
        if obj is None:
            # no valid update; keep loop alive
            await asyncio.sleep(0.01)
            continue

        # Merge with last so partial updates still work
        last.update({k: obj[k] for k in ("x", "y", "mode") if k in obj})

        # If MCU sends LED states too, forward them
        if "led" in obj:
            last["led"] = obj["led"]
        else:
            last.pop("led", None)

        await broadcast(last)

async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True, help="Serial port (e.g., COM5 or /dev/ttyACM0)")
    ap.add_argument("--baud", type=int, default=9600, help="Baud rate (default 9600)")
    ap.add_argument("--ws-host", default="127.0.0.1", help="WebSocket host (default 127.0.0.1)")
    ap.add_argument("--ws-port", type=int, default=8765, help="WebSocket port (default 8765)")
    args = ap.parse_args()

    print(f"[*] WebSocket server on ws://{args.ws_host}:{args.ws_port}")
    async with websockets.serve(ws_handler, args.ws_host, args.ws_port):
        await serial_reader(args.port, args.baud)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[*] Exiting.")