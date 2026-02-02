#!/usr/bin/env python3
"""
SYNAPSE Dashboard Launcher
Starts all components in one click:
- OSC/WebSocket Server
- HTTP Server for dashboard
- Opens browser automatically
"""

import subprocess
import time
import webbrowser
import signal
import sys
import os
from pathlib import Path

# Configuration
OSC_SERVER_SCRIPT = "debug_server.py"
HTTP_PORT = 8000
DASHBOARD_URL = f"http://localhost:{HTTP_PORT}/dashboard.html"

# Process tracking
processes = []

def cleanup(signum=None, frame=None):
    """Clean shutdown of all processes"""
    print("\n\n🛑 Shutting down SYNAPSE Dashboard...")
    for process in processes:
        try:
            process.terminate()
            process.wait(timeout=3)
        except:
            try:
                process.kill()
            except:
                pass
    print("✅ All processes stopped")
    sys.exit(0)

def check_port_available(port):
    """Check if a port is available"""
    import socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind(('', port))
        sock.close()
        return True
    except:
        return False

def main():
    # Register cleanup handler
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    print("=" * 60)
    print("  🧠 SYNAPSE Dashboard Launcher")
    print("=" * 60)
    print()

    # Change to script directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)

    # Check if files exist
    if not Path(OSC_SERVER_SCRIPT).exists():
        print(f"❌ Error: {OSC_SERVER_SCRIPT} not found")
        sys.exit(1)

    if not Path("dashboard.html").exists():
        print("❌ Error: dashboard.html not found")
        sys.exit(1)

    # Check if ports are available
    if not check_port_available(HTTP_PORT):
        print(f"⚠️  Warning: Port {HTTP_PORT} is already in use")
        print(f"   Another HTTP server might be running")
        print()

    # Start OSC/WebSocket server
    print(f"🚀 Starting OSC/WebSocket server...")
    try:
        osc_process = subprocess.Popen(
            ["python3", OSC_SERVER_SCRIPT],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        processes.append(osc_process)
        print("   ✓ OSC listening on port 6970")
        print("   ✓ WebSocket listening on port 8765")
    except Exception as e:
        print(f"   ❌ Failed to start OSC server: {e}")
        cleanup()
        return

    # Give OSC server time to start
    time.sleep(1)

    # Start HTTP server for dashboard
    print(f"🌐 Starting HTTP server on port {HTTP_PORT}...")
    try:
        http_process = subprocess.Popen(
            ["python3", "-m", "http.server", str(HTTP_PORT)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        processes.append(http_process)
        print(f"   ✓ Dashboard available at {DASHBOARD_URL}")
    except Exception as e:
        print(f"   ❌ Failed to start HTTP server: {e}")
        cleanup()
        return

    # Give HTTP server time to start
    time.sleep(1)

    # Open browser
    print("🌍 Opening dashboard in browser...")
    try:
        webbrowser.open(DASHBOARD_URL)
        print("   ✓ Browser opened")
    except Exception as e:
        print(f"   ⚠️  Could not open browser automatically: {e}")
        print(f"   Please open manually: {DASHBOARD_URL}")

    print()
    print("=" * 60)
    print("  ✅ SYNAPSE Dashboard is running!")
    print("=" * 60)
    print()
    print(f"  📊 Dashboard: {DASHBOARD_URL}")
    print(f"  🔌 WebSocket: ws://localhost:8765")
    print(f"  📡 OSC Port: 6970")
    print()
    print("  Press Ctrl+C to stop all services")
    print("=" * 60)
    print()

    # Keep running and monitor processes
    try:
        while True:
            # Check if any process has died
            for process in processes:
                if process.poll() is not None:
                    print(f"\n⚠️  A process has stopped unexpectedly")
                    cleanup()
                    return
            time.sleep(1)
    except KeyboardInterrupt:
        cleanup()

if __name__ == "__main__":
    main()
