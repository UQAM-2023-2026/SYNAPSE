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
from pythonosc import dispatcher, osc_server, udp_client
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

# Config partagée OSC Send (distribuée à tous les clients via WS)
osc_config  = {"messages": [], "target_ip": "127.0.0.1", "target_port": 9000}
config_data = {"current": osc_config, "presets": {}}   # persisted to CONFIG_FILE
CONFIG_FILE = None   # set in main() once script_dir is known

# ─── Config file helpers ───
def load_config():
    """Load current config + presets from JSON file"""
    global osc_config, config_data
    if not CONFIG_FILE or not os.path.exists(CONFIG_FILE):
        return
    try:
        with open(CONFIG_FILE, 'r') as f:
            config_data = json.load(f)
        cur = config_data.get("current", {})
        osc_config["messages"]    = cur.get("messages", [])
        osc_config["target_ip"]   = cur.get("target_ip", "127.0.0.1")
        osc_config["target_port"] = cur.get("target_port", 9000)
    except Exception:
        pass

def save_config():
    """Persist current config + presets to JSON file"""
    if not CONFIG_FILE:
        return
    config_data["current"] = {
        "messages":    osc_config["messages"],
        "target_ip":   osc_config["target_ip"],
        "target_port": osc_config["target_port"]
    }
    try:
        with open(CONFIG_FILE, 'w') as f:
            json.dump(config_data, f, indent=2)
    except Exception:
        pass

def get_preset_names():
    return list(config_data.get("presets", {}).keys())

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

    # Pousse la config OSC + liste presets au nouveau client
    await websocket.send(json.dumps({
        "type":        "osc_config",
        "messages":    osc_config["messages"],
        "target_ip":   osc_config["target_ip"],
        "target_port": osc_config["target_port"],
        "sender_id":   "__server__"
    }))
    await websocket.send(json.dumps({
        "type":    "preset_list",
        "presets": get_preset_names()
    }))

    try:
        async for raw in websocket:
            try:
                msg = json.loads(raw)
                if msg.get("type") == "send_osc":
                    address = msg["address"]
                    value_type = msg.get("value_type", "float")
                    typed_value = int(msg["value"]) if value_type == "int" else float(msg["value"])
                    target_ip = msg.get("target_ip", "127.0.0.1")
                    target_port = int(msg.get("target_port", 9000))
                    client = udp_client.SimpleUDPClient(target_ip, target_port)
                    client.send_message(address, [typed_value])

                elif msg.get("type") == "osc_config_update":
                    osc_config["messages"]    = msg.get("messages", [])
                    osc_config["target_ip"]   = msg.get("target_ip", "127.0.0.1")
                    osc_config["target_port"] = msg.get("target_port", 9000)
                    save_config()
                    # Broadcast à tous (le sender ignore sa propre copie via sender_id)
                    await broadcast(json.dumps({
                        "type":        "osc_config",
                        "messages":    osc_config["messages"],
                        "target_ip":   osc_config["target_ip"],
                        "target_port": osc_config["target_port"],
                        "sender_id":   msg.get("sender_id", "")
                    }))

                elif msg.get("type") == "save_preset":
                    name = msg.get("name", "").strip()
                    if name:
                        config_data.setdefault("presets", {})[name] = {
                            "messages":    [m.copy() for m in osc_config["messages"]],
                            "target_ip":   osc_config["target_ip"],
                            "target_port": osc_config["target_port"]
                        }
                        save_config()
                        await broadcast(json.dumps({
                            "type":    "preset_list",
                            "presets": get_preset_names()
                        }))

                elif msg.get("type") == "load_preset":
                    name = msg.get("name", "")
                    preset = config_data.get("presets", {}).get(name)
                    if preset:
                        osc_config["messages"]    = [m.copy() for m in preset.get("messages", [])]
                        osc_config["target_ip"]   = preset.get("target_ip", "127.0.0.1")
                        osc_config["target_port"] = preset.get("target_port", 9000)
                        save_config()
                        await broadcast(json.dumps({
                            "type":        "osc_config",
                            "messages":    osc_config["messages"],
                            "target_ip":   osc_config["target_ip"],
                            "target_port": osc_config["target_port"],
                            "sender_id":   "__server__"
                        }))

                elif msg.get("type") == "delete_preset":
                    name = msg.get("name", "")
                    if name in config_data.get("presets", {}):
                        del config_data["presets"][name]
                        save_config()
                        await broadcast(json.dumps({
                            "type":    "preset_list",
                            "presets": get_preset_names()
                        }))
            except:
                pass
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

    global CONFIG_FILE
    osc_port = 6970
    ws_port = 8765
    http_port = HTTP_PORT
    local_ip = get_local_ip()
    script_dir = os.path.dirname(os.path.abspath(__file__))

    CONFIG_FILE = os.path.join(script_dir, 'osc_config.json')
    load_config()

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
