#!/usr/bin/env python3
"""
Debug OSC/WebSocket Server with verbose logging
Usage: python debug_server.py [--osc-port PORT] [--ws-port PORT]
"""

import asyncio
import json
import logging
import argparse
import os
from datetime import datetime
from pythonosc import dispatcher, osc_server, udp_client
import websockets

# Very verbose logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - [%(funcName)s] %(message)s'
)
logger = logging.getLogger(__name__)

connected_clients = set()
recent_messages = []
MAX_RECENT_MESSAGES = 20  # Reduced for testing
event_loop = None
message_counter = 0

# ─── OSC Send config + presets (persistés sur fichier) ───
osc_config  = {"messages": [], "target_ip": "127.0.0.1", "target_port": 9000}
config_data = {"current": osc_config, "presets": {}}
CONFIG_FILE = None   # set in main()

def load_config():
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
        logger.info(f"[CONFIG] Loaded: {len(osc_config['messages'])} messages, {len(config_data.get('presets', {}))} presets")
    except Exception as e:
        logger.warning(f"[CONFIG] Failed to load: {e}")

def save_config():
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
    except Exception as e:
        logger.warning(f"[CONFIG] Failed to save: {e}")

def get_preset_names():
    return list(config_data.get("presets", {}).keys())


def osc_handler(address, *args):
    """Handle incoming OSC messages"""
    global message_counter
    message_counter += 1

    timestamp = datetime.now().isoformat()
    message_data = {
        'timestamp': timestamp,
        'address': address,
        'args': list(args)
    }

    recent_messages.append(message_data)
    if len(recent_messages) > MAX_RECENT_MESSAGES:
        recent_messages.pop(0)

    # Only log every 60th message to avoid spam
    if message_counter % 60 == 0:
        logger.info(f"OSC: Received {message_counter} messages total. Current: {address}")

    # Broadcast to WebSocket clients
    if connected_clients and event_loop:
        message_json = json.dumps(message_data)
        future = asyncio.run_coroutine_threadsafe(broadcast_message(message_json), event_loop)
        # Don't wait for result, fire and forget


async def broadcast_message(message):
    """Broadcast message to all connected WebSocket clients"""
    if not connected_clients:
        return

    disconnected = []
    for client in list(connected_clients):
        try:
            await client.send(message)
        except Exception as e:
            logger.warning(f"Failed to send to client: {e}")
            disconnected.append(client)

    for client in disconnected:
        connected_clients.discard(client)
        logger.info(f"Removed disconnected client. Remaining: {len(connected_clients)}")


async def websocket_handler(websocket):
    """Handle WebSocket connections"""
    client_id = f"{websocket.remote_address[0]}:{websocket.remote_address[1]}"
    logger.info(f"[WS] Client connecting from {client_id}")

    connected_clients.add(websocket)
    logger.info(f"[WS] Client {client_id} added. Total clients: {len(connected_clients)}")

    try:
        # Send initial batch of recent messages
        logger.info(f"[WS] Sending {len(recent_messages)} recent messages to {client_id}")
        for idx, msg in enumerate(recent_messages):
            try:
                await websocket.send(json.dumps(msg))
                if idx == 0:
                    logger.debug(f"[WS] First message sent successfully to {client_id}")
            except Exception as e:
                logger.error(f"[WS] Error sending message {idx} to {client_id}: {e}")
                raise

        # Pousse config OSC + liste presets
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

        logger.info(f"[WS] Initial batch sent to {client_id}. Connection will stay open...")

        # Listen for incoming messages
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
                    logger.debug(f"[OSC SEND] {address} = {typed_value} ({value_type}) → {target_ip}:{target_port}")

                elif msg.get("type") == "osc_config_update":
                    osc_config["messages"]    = msg.get("messages", [])
                    osc_config["target_ip"]   = msg.get("target_ip", "127.0.0.1")
                    osc_config["target_port"] = msg.get("target_port", 9000)
                    save_config()
                    await broadcast_message(json.dumps({
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
                        logger.info(f"[PRESET] Saved: '{name}'")
                        await broadcast_message(json.dumps({
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
                        logger.info(f"[PRESET] Loaded: '{name}'")
                        await broadcast_message(json.dumps({
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
                        logger.info(f"[PRESET] Deleted: '{name}'")
                        await broadcast_message(json.dumps({
                            "type":    "preset_list",
                            "presets": get_preset_names()
                        }))

            except Exception as e:
                logger.error(f"[WS] Error processing message from {client_id}: {e}")

    except websockets.exceptions.ConnectionClosedOK:
        logger.info(f"[WS] Client {client_id} closed connection normally")
    except websockets.exceptions.ConnectionClosedError as e:
        logger.error(f"[WS] Client {client_id} connection error: {e}")
    except Exception as e:
        logger.error(f"[WS] Unexpected error with {client_id}: {type(e).__name__}: {e}")
        import traceback
        traceback.print_exc()
    finally:
        connected_clients.discard(websocket)
        logger.info(f"[WS] Client {client_id} disconnected. Total clients: {len(connected_clients)}")


def start_osc_server(ip, port):
    """Start OSC server"""
    logger.info(f"[OSC] Starting OSC server on {ip}:{port}")
    disp = dispatcher.Dispatcher()
    disp.map("/*", osc_handler)

    server = osc_server.ThreadingOSCUDPServer((ip, port), disp)
    logger.info(f"[OSC] OSC Server listening and ready")

    server.serve_forever()


async def start_websocket_server(ip, port):
    """Start WebSocket server"""
    logger.info(f"[WS] Starting WebSocket server on {ip}:{port}")
    async with websockets.serve(websocket_handler, ip, port):
        logger.info(f"[WS] WebSocket Server listening and ready")
        await asyncio.Future()


async def main(osc_port, ws_port):
    """Main function"""
    global event_loop, CONFIG_FILE

    event_loop = asyncio.get_running_loop()

    CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'osc_config.json')
    load_config()

    logger.info("=" * 60)
    logger.info("SYNAPSE OSC DEBUG SERVER")
    logger.info("=" * 60)

    osc_ip = "0.0.0.0"
    ws_ip = "0.0.0.0"

    logger.info(f"OSC will listen on: {osc_ip}:{osc_port}")
    logger.info(f"WebSocket will listen on: {ws_ip}:{ws_port}")
    logger.info("=" * 60)

    # Start OSC server in thread
    import threading
    osc_thread = threading.Thread(
        target=start_osc_server,
        args=(osc_ip, osc_port),
        daemon=True
    )
    osc_thread.start()

    # Small delay to ensure OSC thread starts
    await asyncio.sleep(0.5)

    # Start WebSocket server
    logger.info(f"[WS] Starting WebSocket server on {ws_ip}:{ws_port}")
    server = await websockets.serve(websocket_handler, ws_ip, ws_port)
    logger.info(f"[WS] WebSocket Server listening and ready")
    await server.wait_closed()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='SYNAPSE OSC/WebSocket Server')
    parser.add_argument('--osc-port', type=int, default=6970, help='OSC listening port (default: 6970)')
    parser.add_argument('--ws-port', type=int, default=8765, help='WebSocket port (default: 8765)')
    args = parser.parse_args()

    try:
        asyncio.run(main(args.osc_port, args.ws_port))
    except KeyboardInterrupt:
        logger.info("\nServer stopped by user")
