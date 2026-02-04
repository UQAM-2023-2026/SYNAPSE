#!/usr/bin/env python3
"""
SYNAPSE Dashboard Launcher
Starts all components in one click:
- OSC/WebSocket Server
- HTTP Server for dashboard
- Opens browser automatically

Usage: python launch.py [--osc-port PORT] [--ws-port PORT] [--http-port PORT]
"""

import subprocess
import time
import webbrowser
import signal
import sys
import os
import argparse
from pathlib import Path

# Default Configuration
OSC_SERVER_SCRIPT = "debug_server.py"
DEFAULT_OSC_PORT = 6970
DEFAULT_WS_PORT = 8765
DEFAULT_HTTP_PORT = 8000

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
    # Parse arguments
    parser = argparse.ArgumentParser(description='SYNAPSE Dashboard Launcher')
    parser.add_argument('--osc-port', type=int, default=DEFAULT_OSC_PORT, help=f'OSC listening port (default: {DEFAULT_OSC_PORT})')
    parser.add_argument('--ws-port', type=int, default=DEFAULT_WS_PORT, help=f'WebSocket port (default: {DEFAULT_WS_PORT})')
    parser.add_argument('--http-port', type=int, default=DEFAULT_HTTP_PORT, help=f'HTTP server port (default: {DEFAULT_HTTP_PORT})')
    parser.add_argument('--no-browser', action='store_true', help='Do not open browser automatically')
    args = parser.parse_args()

    osc_port = args.osc_port
    ws_port = args.ws_port
    http_port = args.http_port
    dashboard_url = f"http://localhost:{http_port}/dashboard.html"

    # Register cleanup handler
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    print("=" * 60)
    print("  SYNAPSE Dashboard Launcher")
    print("=" * 60)
    print()

    # Change to script directory
    script_dir = Path(__file__).parent
    os.chdir(script_dir)

    # Check if files exist
    if not Path(OSC_SERVER_SCRIPT).exists():
        print(f"[ERROR] {OSC_SERVER_SCRIPT} not found")
        sys.exit(1)

    if not Path("dashboard.html").exists():
        print("[ERROR] dashboard.html not found")
        sys.exit(1)

    # Find available HTTP port
    original_http_port = http_port
    while not check_port_available(http_port) and http_port < original_http_port + 10:
        http_port += 1

    if http_port != original_http_port:
        print(f"[INFO] Port {original_http_port} busy, using port {http_port}")

    dashboard_url = f"http://localhost:{http_port}/dashboard.html"

    # Use the same Python executable that's running this script
    python_cmd = sys.executable

    # Start OSC/WebSocket server
    print(f"[START] OSC/WebSocket server...")
    try:
        osc_process = subprocess.Popen(
            [python_cmd, OSC_SERVER_SCRIPT, f"--osc-port={osc_port}", f"--ws-port={ws_port}"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE
        )
        processes.append(osc_process)

        # Give it a moment to start and check if it crashed immediately
        time.sleep(1.5)
        if osc_process.poll() is not None:
            stderr = osc_process.stderr.read().decode() if osc_process.stderr else "Unknown error"
            print(f"   [FAIL] OSC server crashed on startup")
            print(f"   [ERROR] {stderr[:500]}")
            sys.exit(1)

        print(f"   [OK] OSC listening on port {osc_port}")
        print(f"   [OK] WebSocket listening on port {ws_port}")
    except Exception as e:
        print(f"   [FAIL] Failed to start OSC server: {e}")
        sys.exit(1)

    # Start HTTP server for dashboard
    print(f"[START] HTTP server on port {http_port}...")
    try:
        http_process = subprocess.Popen(
            [python_cmd, "-m", "http.server", str(http_port)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE
        )
        processes.append(http_process)

        # Give it time to start
        time.sleep(0.5)
        if http_process.poll() is not None:
            stderr = http_process.stderr.read().decode() if http_process.stderr else "Unknown error"
            print(f"   [FAIL] HTTP server failed: {stderr[:200]}")
            cleanup()
            return

        print(f"   [OK] Dashboard available at {dashboard_url}")
    except Exception as e:
        print(f"   [FAIL] Failed to start HTTP server: {e}")
        cleanup()
        return

    # Open browser
    if not args.no_browser:
        print("[START] Opening dashboard in browser...")
        try:
            webbrowser.open(dashboard_url)
            print("   [OK] Browser opened")
        except Exception as e:
            print(f"   [WARN] Could not open browser automatically: {e}")
            print(f"   Please open manually: {dashboard_url}")

    print()
    print("=" * 60)
    print("  SYNAPSE Dashboard is running!")
    print("=" * 60)
    print()
    print(f"  Dashboard: {dashboard_url}")
    print(f"  WebSocket: ws://localhost:{ws_port}")
    print(f"  OSC Port:  {osc_port}")
    print()
    print("  Press Ctrl+C to stop all services")
    print("=" * 60)
    print()

    # Keep running and monitor processes
    try:
        while True:
            # Check if any process has died
            for i, process in enumerate(processes):
                retcode = process.poll()
                if retcode is not None:
                    proc_name = "OSC/WebSocket server" if i == 0 else "HTTP server"
                    print(f"\n[WARN] {proc_name} stopped (exit code: {retcode})")
                    cleanup()
                    return
            time.sleep(2)
    except KeyboardInterrupt:
        cleanup()

if __name__ == "__main__":
    main()
