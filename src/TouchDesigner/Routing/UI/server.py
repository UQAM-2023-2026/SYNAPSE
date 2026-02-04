#!/usr/bin/env python3
"""
SYNAPSE Server - Simple OSC to WebSocket bridge + HTTP server
"""

import asyncio
import json
import threading
import http.server
import socketserver
import os
import socket
import time
from datetime import datetime
from pythonosc import dispatcher, osc_server
import websockets

# Global state
clients = set()
loop = None
HTTP_PORT = 9696

# Throttling par adresse
RATE_60HZ = 1/60  # LFOs
RATE_45HZ = 1/45  # Autres
LFO_ADDRESSES = {'/S_Breath', '/I_Breath'}
last_sent = {}  # {address: timestamp}

def handle_osc(address, *args):
    """Forward OSC to WebSocket clients (with per-address throttling)"""
    if not clients or not loop:
        return

    now = time.time()

    # Determine rate based on address type
    min_interval = RATE_60HZ if address in LFO_ADDRESSES else RATE_45HZ

    # Check if enough time has passed since last send for this address
    if address in last_sent:
        if now - last_sent[address] < min_interval:
            return  # Skip, too soon

    last_sent[address] = now

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

def run_http(port, directory):
    """Run HTTP server in thread"""
    os.chdir(directory)
    handler = http.server.SimpleHTTPRequestHandler
    handler.log_message = lambda *args: None  # Silence logs

    # Try multiple ports if needed
    for try_port in [port, port+1, port+2, port+3]:
        try:
            with socketserver.TCPServer(("0.0.0.0", try_port), handler) as httpd:
                print(f"[HTTP] Listening on port {try_port}")
                httpd.serve_forever()
                break
        except OSError:
            continue

def get_local_ip():
    """Get local network IP"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "localhost"

async def main():
    global loop
    loop = asyncio.get_running_loop()

    osc_port = 6970
    ws_port = 8765
    http_port = HTTP_PORT
    local_ip = get_local_ip()
    script_dir = os.path.dirname(os.path.abspath(__file__))

    print("=" * 60)
    print("  SYNAPSE Server")
    print("=" * 60)
    print(f"  Local IP:  {local_ip}")
    print("=" * 60)
    print(f"  OSC:       port {osc_port}")
    print(f"  WebSocket: port {ws_port}")
    print(f"  HTTP:      port {http_port}")
    print("=" * 60)
    print(f"  Dashboard (local):   http://localhost:{http_port}/dashboard.html")
    print(f"  Dashboard (network): http://{local_ip}:{http_port}/dashboard.html")
    print("=" * 60)
    print()

    # Start OSC in background thread
    threading.Thread(target=run_osc, args=(osc_port,), daemon=True).start()

    # Start HTTP in background thread
    threading.Thread(target=run_http, args=(http_port, script_dir), daemon=True).start()

    # Start WebSocket server
    print(f"[WS] Listening on port {ws_port}")
    async with websockets.serve(ws_handler, "0.0.0.0", ws_port):
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopped")
