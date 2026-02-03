#!/usr/bin/env python3
"""
SYNAPSE Server - Simple OSC to WebSocket bridge
"""

import asyncio
import json
import threading
from datetime import datetime
from pythonosc import dispatcher, osc_server
import websockets

# Global state
clients = set()
loop = None

def handle_osc(address, *args):
    """Forward OSC to WebSocket clients"""
    if not clients or not loop:
        return

    msg = json.dumps({
        'timestamp': datetime.now().isoformat(),
        'address': address,
        'args': list(args)
    })

    asyncio.run_coroutine_threadsafe(broadcast(msg), loop)

async def broadcast(msg):
    """Send to all clients"""
    dead = set()
    for ws in clients.copy():
        try:
            await ws.send(msg)
        except:
            dead.add(ws)
    clients.difference_update(dead)

async def ws_handler(websocket):
    """Handle WebSocket client"""
    clients.add(websocket)
    print(f"[+] Client connected ({len(clients)} total)")
    try:
        await websocket.wait_closed()
    finally:
        clients.discard(websocket)
        print(f"[-] Client disconnected ({len(clients)} total)")

def run_osc(port):
    """Run OSC server in thread"""
    d = dispatcher.Dispatcher()
    d.map("/*", handle_osc)
    server = osc_server.ThreadingOSCUDPServer(("0.0.0.0", port), d)
    print(f"[OSC] Listening on port {port}")
    server.serve_forever()

async def main():
    global loop
    loop = asyncio.get_running_loop()

    osc_port = 6970
    ws_port = 8765

    print("=" * 50)
    print("  SYNAPSE Server")
    print("=" * 50)
    print(f"  OSC:       0.0.0.0:{osc_port}")
    print(f"  WebSocket: 0.0.0.0:{ws_port}")
    print("=" * 50)

    # Start OSC in background thread
    threading.Thread(target=run_osc, args=(osc_port,), daemon=True).start()

    # Start WebSocket server
    print(f"[WS] Listening on port {ws_port}")
    async with websockets.serve(ws_handler, "0.0.0.0", ws_port):
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopped")
