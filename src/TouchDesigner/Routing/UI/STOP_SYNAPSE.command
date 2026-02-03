#!/bin/bash
echo "Stopping SYNAPSE server..."
pkill -f "python3 server.py" 2>/dev/null
pkill -f "python server.py" 2>/dev/null
echo "Done."
