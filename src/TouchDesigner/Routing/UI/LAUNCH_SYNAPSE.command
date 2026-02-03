#!/bin/bash
# SYNAPSE Dashboard Launcher - Double-click to start

# Change to script directory
cd "$(dirname "$0")"

# Run launcher
caffeinate -ims python3 launch.py
