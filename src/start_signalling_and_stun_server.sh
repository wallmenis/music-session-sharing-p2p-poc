#!/bin/bash

turnserver --no-auth --verbose &
sudo nmcli device down enp1s0
trap : INT
tmux -c /home/wallmenis/Desktop/libdatachannel/examples/signaling-server-python/signaling-server.py
kill $(pgrep turnserver)
sudo nmcli device up enp1s0
