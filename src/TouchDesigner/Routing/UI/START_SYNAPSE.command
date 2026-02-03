#!/bin/bash
cd "$(dirname "$0")"

echo "============================================================"
echo "  SYNAPSE Server"
echo "============================================================"
echo ""
echo "  OSC Port:  6970"
echo "  WebSocket: 8765"
echo ""
echo "  Dashboard: Ouvre dashboard.html dans ton navigateur"
echo ""
echo "  Press Ctrl+C to stop"
echo "============================================================"
echo ""

python3 server.py
