#!/usr/bin/env python3
"""
Debug OSC/WebSocket Server with verbose logging
"""

import asyncio
import json
import logging
from datetime import datetime
from pythonosc import dispatcher, osc_server
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

        logger.info(f"[WS] Initial batch sent to {client_id}. Connection will stay open...")

        # Keep connection alive indefinitely - wait for websocket to close
        # This will block until the client disconnects
        await websocket.wait_closed()

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


async def main():
    """Main function"""
    global event_loop

    event_loop = asyncio.get_event_loop()
    logger.info("=" * 60)
    logger.info("SYNAPSE OSC DEBUG SERVER")
    logger.info("=" * 60)

    osc_ip = "0.0.0.0"
    osc_port = 6970
    ws_ip = "0.0.0.0"
    ws_port = 8765

    logger.info(f"OSC will listen on: {osc_ip}:{osc_port}")
    logger.info(f"WebSocket will listen on: {ws_ip}:{ws_port}")
    logger.info(f"Web UI: http://10.0.2.245:8000")
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
    await start_websocket_server(ws_ip, ws_port)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("\nServer stopped by user")
